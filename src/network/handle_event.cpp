//
//  handle_event.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 21/07/2023.
//

#include "handle_event.hpp"
#include "network.hpp"
#include "../models/profile.hpp"
#include "../data_layer/accounts.hpp"
#include "../data_layer/conversations.hpp"
#include "../data_layer/profiles.hpp"
#include "../data_layer/contact_lists.hpp"
#include <string.h>
#include <stdio.h>

static void handle_kind_0(const char* subscription_id, Event* event);
static void handle_kind_3(const char* subscription_id, Event* event);

void network::handle_event(const char* subscription_id, Event* event_) {

    Event* event = (Event*)malloc(Event::size_of(event_));
    memcpy(event, event_, Event::size_of(event_));

    if (!event_validate(event)) {
        printf("event invalid: %s\n", event->validity == EVENT_INVALID_ID ? "INVALID_ID" : "INVALID_SIG");
        return;
    }

    if (event->kind == 0) {
        handle_kind_0(subscription_id, event);
    } else if (event->kind == 3) {
        handle_kind_3(subscription_id, event);
    } else if (event->kind == 4) {
        data_layer::receive_direct_message(event);
    } else {
        printf("received event kind %d, which we don't handle current...\n", event->kind);
    }

}

static void handle_kind_0(const char* subscription_id, Event* event) {

    Profile* profile = (Profile*)malloc(Profile::size_from_event(event));

    if (!parse_profile_data(profile, event)) {
        printf("Invalid profile data :(\n");
        printf("%s\n", event->content.data.get(event));
        free(profile);
        return;
    }

    data_layer::profiles.push_back(profile);

    ui::redraw();
}

static void handle_kind_3(const char* subscription_id, Event* event) {

    // Add the contact list
    bool replaced_older = false;
    for (auto i = 0; i < data_layer::contact_lists.size(); ++i) {
        if (compare_keys(&data_layer::contact_lists[i]->pubkey, &event->pubkey)) {
            if (data_layer::contact_lists[i]->created_at > event->created_at) {
                return;
            }
            data_layer::contact_lists[i] = event;
            replaced_older = true;
            break;
        }
    }
    if (!replaced_older) {
        data_layer::contact_lists.push_back(event);
    }

    // Is it our contact list?
    if (compare_keys(&event->pubkey, &data_layer::accounts[data_layer::account_selected].pubkey)) {
        auto p_tags = event->p_tags.get(event);
        for (auto& p_tag : p_tags) {
            data_layer::request_profile(&p_tag.pubkey);
        }
        data_layer::batch_profile_requests_send();
    }

    ui::redraw();
}
