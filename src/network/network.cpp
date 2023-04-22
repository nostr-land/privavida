//
//  network.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 20/07/2023.
//

#include "network.hpp"
#include <app.h>
#include "handle_event.hpp"
#include "../models/event_parse.hpp"
#include "../models/event_stringify.hpp"
#include "../models/relay_message_parse.hpp"
#include "../models/hex.hpp"
#include <string.h>

static AppNetworking networking;

void network::init(AppNetworking networking_) {
    networking = networking_;

    if (data_layer::accounts.empty()) {
        return;
    }
    account_set_response_callback(&network::account_response_handler);

    auto socket = networking.websocket_open(networking.opaque_ptr, "wss://relay.damus.io");
}

void app_websocket_event(const AppWebsocketEvent* event) {

    auto account = data_layer::accounts[data_layer::account_selected];

    if (event->type == WEBSOCKET_OPEN) {
        printf("websocket open!\n");

        char pubkey_hex[65];
        hex_encode(pubkey_hex, account.pubkey.data, sizeof(Pubkey));
        pubkey_hex[64] = '\0';

        char req[128];
        sprintf(req, "[\"REQ\",\"dms_sent\",{\"authors\":[\"%s\"],\"kinds\":[4]}]", pubkey_hex);
        printf("Request: %s\n", req);
        networking.websocket_send(networking.opaque_ptr, event->ws, req);

        sprintf(req, "[\"REQ\",\"dms_received\",{\"#p\":[\"%s\"],\"kinds\":[4]}]", pubkey_hex);
        printf("Request: %s\n", req);
        networking.websocket_send(networking.opaque_ptr, event->ws, req);

        sprintf(req, "[\"REQ\",\"prof\",{\"authors\":[\"%s\"],\"kinds\":[0]}]", pubkey_hex);
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
        printf("other message: %s\n", event->data);
        return;
    }

    EventParseResult res;
    err = event_parse(message.event.input, message.event.input_len, buffer, res);
    if (err) {
        printf("event invalid (parse error): %d\n", (int)err);
        printf("data: %s\n", message.event.input);
        return;
    }

    uint8_t nostr_event_buf[res.event_size];
    Event* nostr_event = (Event*)nostr_event_buf;
    event_create(nostr_event, buffer, res);

    network::handle_event(message.event.subscription_id, nostr_event);
}

void network::send(const char* message) {
    networking.websocket_send(networking.opaque_ptr, 0, message);
}
