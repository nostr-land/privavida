//
//  profiles.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-22.
//

#include "profiles.hpp"
#include "../network/network.hpp"
#include "../models/hex.hpp"
#include "../models/nostr_entity.hpp"
#include "../models/filters.hpp"
#include <string.h>
#include <stdio.h>
#include <rapidjson/writer.h>

namespace data_layer {

static std::vector<Profile*> profiles;
static std::vector<Pubkey> profiles_requested;
static bool is_batching = false;

void receive_profile(EventLocator event_loc) {
    auto event = data_layer::event(event_loc);

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

const Profile* get_profile(const Pubkey* pubkey) {
    for (auto profile : profiles) {
        if (compare_keys(&profile->pubkey, pubkey)) {
            return profile;
        }
    }
    return NULL;
}

const Profile* get_or_request_profile(const Pubkey* pubkey) {
    for (auto profile : profiles) {
        if (compare_keys(&profile->pubkey, pubkey)) {
            return profile;
        }
    }
    request_profile(pubkey);
    return NULL;
}

static std::vector<Pubkey> batched_requests;

static void send_batch();

void request_profile(const Pubkey* pubkey) {
    for (auto& other_pubkey : profiles_requested) {
        if (compare_keys(&other_pubkey, pubkey)) {
            return;
        }
    }

    profiles_requested.push_back(*pubkey);

    bool found = false;
    for (auto& other_pubkey : batched_requests) {
        if (compare_keys(&other_pubkey, pubkey)) {
            found = true;
            break;
        }
    }
    if (!found) {
        batched_requests.push_back(*pubkey);
    }

    if (!is_batching) {
        send_batch();
    }
}

void batch_profile_requests() {
    is_batching = true;
}

void batch_profile_requests_send() {
    is_batching = false;
    send_batch();
}

void send_batch() {
    if (batched_requests.empty()) {
        return;
    }

    static int count = 0;
    char sub_id[32];
    snprintf(sub_id, sizeof(sub_id), "prof_%d", count++);

    StackBufferFixed<256> filters_buffer;
    auto filters = FiltersBuilder(&filters_buffer)
        .kind(0)
        .authors((uint32_t)batched_requests.size(), &batched_requests[0])
        .get();

    network::subscribe(sub_id, filters, false);

    batched_requests.clear();
}

}
