//
//  network.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 20/07/2023.
//

#include "network.hpp"
#include <app.hpp>
#include <platform.h>
#include "../models/event_parse.hpp"
#include "../models/event_stringify.hpp"
#include "../models/relay_message.hpp"
#include "../models/client_message.hpp"
#include "../models/hex.hpp"
#include "../data_layer/profiles.hpp"
#include "../data_layer/accounts.hpp"
#include "../data_layer/events.hpp"
#include <string.h>

static std::vector<AppWebsocketHandle> sockets;
static std::vector<char*> subs_to_keep_alive;

void network::init() {
    if (!data_layer::current_account()) {
        return;
    }
    if (!sockets.empty()) {
        auto old_sockets = sockets;
        sockets.clear();
        for (auto socket : old_sockets) {
            platform_websocket_close(socket, 1000, "");
        }
    }

    platform_websocket_open("wss://relay.snort.social", NULL);
    platform_websocket_open("wss://relay.damus.io", NULL);
    platform_websocket_open("wss://eden.nostr.land", NULL);
}

void app_websocket_event(const AppWebsocketEvent* event) {

    if (event->type == WEBSOCKET_OPEN) {
        printf("websocket open!\n");
        sockets.push_back(event->socket);

        auto pubkey = data_layer::current_account()->pubkey;

        StackBufferFixed<128> text_buffer;
        StackBufferFixed<128> filters_buffer;

        // "dms_sent" subscription
        {
            auto filters = FiltersBuilder(&filters_buffer)
                .kind(4)
                .author(&pubkey)
                .get();

            network::subscribe("dms_sent", filters, false);
        }

        // "dms_received" subscription
        {
            auto filters = FiltersBuilder(&filters_buffer)
                .kind(4)
                .p_tag(&pubkey)
                .get();

            network::subscribe("dms_recv", filters, false);
        }

        // "profile" subscription (fetches kind 0 metadata and kind 3 contact list)
        {
            uint32_t kinds[2] = { 0, 3 };
            auto filters = FiltersBuilder(&filters_buffer)
                .kinds(2, kinds)
                .author(&pubkey)
                .get();

            network::subscribe("profile", filters, true);
        }

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

    if (message.type == RelayMessage::EOSE) {
        printf("eose: %s\n", event->data);
        bool should_keep_alive = false;
        for (auto sub_id : subs_to_keep_alive) {
            if (strcmp(message.eose.subscription_id, sub_id) == 0) {
                should_keep_alive = true;
                break;
            }
        }
        if (!should_keep_alive) {
            StackBufferFixed<64> buffer;
            auto req = client_message_close(message.eose.subscription_id, &buffer);
            printf("Request: %s\n", req);
            platform_websocket_send(event->socket, req);
        }
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

    data_layer::receive_event(nostr_event);
}

void network::subscribe(const char* sub_id, const Filters* filters, bool unsub_after_eose) {
    StackBufferFixed<256> buffer;

    auto req = client_message_req(sub_id, filters, &buffer);
    printf("Request: %s\n", req);
    for (auto& ws : sockets) {
        platform_websocket_send(ws, req);
    }

    if (!unsub_after_eose) {
        auto sub_id_len = strlen(sub_id) + 1;
        auto sub_id_copy = (char*)malloc(sub_id_len);
        strncpy(sub_id_copy, sub_id, sub_id_len);
        subs_to_keep_alive.push_back(sub_id_copy);
    }
}

void network::send(const char* message) {
    printf("Request: %s\n", message);
    for (auto& ws : sockets) {
        platform_websocket_send(ws, message);
    }
}

void network::fetch(const char* url, network::FetchCallback callback) {
    auto cb = new network::FetchCallback(std::move(callback));
    platform_http_request(url, cb);
}

void network::fetch_image(const char* url, network::FetchImageCallback callback) {
    auto cb = new network::FetchImageCallback(std::move(callback));
    platform_http_image_request(url, cb);
}

void app_http_response(int status_code, const unsigned char* data, int data_length, void* user_data) {
    auto cb = (network::FetchCallback*)user_data;
    (*cb)(status_code == -1, status_code, data, data_length);
    delete cb;
}

void app_http_image_response(int image_id, void* user_data) {
    auto cb = (network::FetchImageCallback*)user_data;
    (*cb)(image_id);
    delete cb;
}
