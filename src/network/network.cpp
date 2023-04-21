//
//  network.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 20/07/2023.
//

#include "network.hpp"
#include "../app.h"
#include "../models/event_parse.hpp"
#include "../models/event_stringify.hpp"
#include "../models/relay_message_parse.hpp"
#include "../models/hex.hpp"
#include "../nostr/accounts.hpp"
#include <string.h>

static AppNetworking networking;
std::vector<Event*> network::events;

static void account_response_cb(const AccountResponse* event);

void network::init(AppNetworking networking_) {
    networking = networking_;

    if (!accounts.size) {
        return;
    }
    account_response_callback = &account_response_cb;

    auto socket = networking.websocket_open(networking.opaque_ptr, "wss://relay.damus.io");
}

void app_websocket_event(const AppWebsocketEvent* event) {

    auto account = accounts[account_selected];

    if (event->type == WEBSOCKET_OPEN) {
        printf("websocket open!\n");

        char pubkey_hex[65];
        hex_encode(pubkey_hex, account->pubkey.data, sizeof(Pubkey));
        pubkey_hex[64] = '\0';

        char req[128];
        sprintf(req, "[\"REQ\",\"sub\",{\"authors\":[\"%s\"],\"kinds\":[4]}]", pubkey_hex);
        printf("Request: %s\n", req);
        networking.websocket_send(networking.opaque_ptr, event->ws, req);

    } else if (event->type == WEBSOCKET_CLOSE) {
        printf("websocket close!\n");
    } else if (event->type == WEBSOCKET_ERROR) {
        printf("websocket error!\n");
    } else if (event->type != WEBSOCKET_MESSAGE) {
        return;
    }

    if (event->data_length == 0) {
        printf("received empty string\n");
        return;
    }

    uint8_t buffer[event->data_length];

    ParseError err;

    RelayMessage message;
    if (!relay_message_parse(event->data, event->data_length, buffer, &message)) {
        printf("message parse error\n");
        return;
    }

    if (message.type != RelayMessage::EVENT) {
        printf("other message: %d\n", (int)message.type);
        printf("data: %s\n", event->data);
        return;
    }

    EventParseResult res;
    err = event_parse(message.event.input, message.event.input_len, buffer, res);
    if (err) {
        printf("event invalid (parse error): %d\n", (int)err);
        printf("data: %s\n", message.event.input);
        return;
    }

    Event* nostr_event = (Event*)malloc(res.event_size);
    event_create(nostr_event, buffer, res);

    if (!event_validate(nostr_event)) {
        printf("event invalid: %s\n", nostr_event->validity == EVENT_INVALID_ID ? "INVALID_ID" : "INVALID_SIG");
        printf("data: %s\n", message.event.input);
        return;
    }

    if (nostr_event->kind == 4) {
        nostr_event->content_encryption = EVENT_CONTENT_ENCRYPTED;

        Pubkey pubkey;
        if (!event_get_first_p_tag(nostr_event, &pubkey)) {
            pubkey = account->pubkey; // Try with our own pubkey if we don't find a p-tag
        }

        auto ciphertext = nostr_event->content.data.get(nostr_event);
        auto len = nostr_event->content.size;
        account_nip04_decrypt(account, &pubkey, ciphertext, len, nostr_event);
    }

    network::events.push_back(nostr_event);
    ui::redraw();
}

void account_response_cb(const AccountResponse* response) {
    switch (response->action) {
        case AccountResponse::SIGN_EVENT: {
            printf("signed event!\n");
            break;
        }
        case AccountResponse::NIP04_ENCRYPT: {
            break;
        }
        case AccountResponse::NIP04_DECRYPT: {
            Event* event = (Event*)response->user_data;

            // Get the result
            const char* result;
            if (response->error) {
                event->content_encryption = EVENT_CONTENT_DECRYPT_FAILED;
                return;
            }

            // Copy over the decrypted data
            auto decrypted = response->result_nip04;
            auto decrypted_len = response->result_nip04_len;
            if (decrypted_len > event->content.size) {
                printf("NIP04: Decoded plaintext longer than encoded ciphertext!!!\n");
                event->content_encryption = EVENT_CONTENT_DECRYPT_FAILED;
                return;
            }

            event->content_encryption = EVENT_CONTENT_DECRYPTED;
            memcpy(event->content.data.get(event), decrypted, decrypted_len);
            event->content.data.get(event)[decrypted_len] = '\0';
            event->content.size = decrypted_len;
            break;
        }
    }
}
