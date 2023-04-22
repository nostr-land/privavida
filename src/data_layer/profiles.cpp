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
#include <string.h>
#include <stdio.h>
#include <rapidjson/writer.h>

namespace data_layer {

std::vector<Profile*> profiles;
static std::vector<Pubkey> profiles_requested;
static bool is_batching = false;

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

    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);

    writer.StartArray();

    writer.String("REQ");

    char sub_id[32];
    snprintf(sub_id, sizeof(sub_id), "prof_%d", count++);
    writer.String(sub_id);

    writer.StartObject();

    writer.String("authors");
    writer.StartArray();

    for (auto& pubkey : batched_requests) {
        char pubkey_hex[65];
        hex_encode(pubkey_hex, pubkey.data, sizeof(Pubkey));
        pubkey_hex[64] = '\0';
        writer.String(pubkey_hex);
    }

    writer.EndArray();

    writer.String("kinds");
    writer.StartArray();
    writer.Int(0);
    writer.EndArray();

    writer.EndObject();
    writer.EndArray();

    network::send(sb.GetString());

    batched_requests.clear();
}

}
