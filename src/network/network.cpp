//
//  network.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 20/07/2023.
//

#include "network.hpp"
#include <app.hpp>
#include <platform.h>
#include "handle_event.hpp"
#include "../models/event_parse.hpp"
#include "../models/event_stringify.hpp"
#include "../models/relay_message_parse.hpp"
#include "../models/hex.hpp"
#include "../data_layer/profiles.hpp"
#include <string.h>

static std::vector<AppWebsocketHandle> sockets;

void network::init() {
    if (data_layer::accounts.empty()) {
        return;
    }

    platform_websocket_open("wss://relay.snort.social", NULL);
    platform_websocket_open("wss://relay.damus.io", NULL);
    platform_websocket_open("wss://eden.nostr.land", NULL);
}

void app_websocket_event(const AppWebsocketEvent* event) {

    auto account = data_layer::accounts[data_layer::account_selected];

    if (event->type == WEBSOCKET_OPEN) {
        printf("websocket open!\n");
        sockets.push_back(event->socket);

        char pubkey_hex[65];
        hex_encode(pubkey_hex, account.pubkey.data, sizeof(Pubkey));
        pubkey_hex[64] = '\0';

        char req[128];
        snprintf(req, 128, "[\"REQ\",\"dms_sent\",{\"authors\":[\"%s\"],\"kinds\":[4]}]", pubkey_hex);
        printf("Request: %s\n", req);
        platform_websocket_send(event->socket, req);

        snprintf(req, 128, "[\"REQ\",\"dms_received\",{\"#p\":[\"%s\"],\"kinds\":[4]}]", pubkey_hex);
        printf("Request: %s\n", req);
        platform_websocket_send(event->socket, req);

        snprintf(req, 128, "[\"REQ\",\"profile\",{\"authors\":[\"%s\"],\"kinds\":[0,3]}]", pubkey_hex);
        printf("Request: %s\n", req);
        platform_websocket_send(event->socket, req);

        data_layer::batch_profile_requests();
        return;

    } else if (event->type == WEBSOCKET_CLOSE) {
        printf("websocket close!\n");
        for (auto i = 0; i < sockets.size(); ++i) {
            if (sockets[i] == event->socket) {
                sockets.erase(sockets.begin() + i);
                break;
            }
        }
        return;
    } else if (event->type == WEBSOCKET_ERROR) {
        printf("websocket error!\n");
        for (auto i = 0; i < sockets.size(); ++i) {
            if (sockets[i] == event->socket) {
                sockets.erase(sockets.begin() + i);
                break;
            }
        }
        return;
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
    for (auto& ws : sockets) {
        platform_websocket_send(ws, message);
    }
}

void network::fetch(const char* url, network::FetchCallback callback) {
    auto cb = new network::FetchCallback(std::move(callback));
    platform_http_request_send(url, cb);
}

void app_http_event(const AppHttpEvent* event) {
    auto cb = (network::FetchCallback*)event->user_data;
    auto err = (event->type == HTTP_RESPONSE_ERROR);
    (*cb)(err, event->status_code, event->data, event->data_length);
    delete cb;
}
