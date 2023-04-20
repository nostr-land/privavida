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

static AppNetworking networking;
std::vector<Event*> network::events;

void network::init(AppNetworking networking_) {
    networking = networking_;
    auto socket = networking.websocket_open(networking.opaque_ptr, "wss://relay.damus.io");
}

void app_websocket_event(const AppWebsocketEvent* event) {

    if (event->type == WEBSOCKET_OPEN) {
        printf("websocket open!\n");
        const char* req = "[\"REQ\",\"sub\",{\"authors\":[\"489ac583fc30cfbee0095dd736ec46468faa8b187e311fda6269c4e18284ed0c\"],\"kinds\":[1]}]";
        networking.websocket_send(networking.opaque_ptr, event->ws, req);
    } else if (event->type == WEBSOCKET_CLOSE) {
        printf("websocket close!\n");
    } else if (event->type == WEBSOCKET_ERROR) {
        printf("websocket error!\n");
    } else if (event->type != WEBSOCKET_MESSAGE) {
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

    network::events.push_back(nostr_event);
    ui::redraw();
}
