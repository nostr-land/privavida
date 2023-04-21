//
//  keys.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-21.
//

#pragma once
#include <inttypes.h>

struct EventId {
    uint8_t data[32];
};

struct Pubkey {
    uint8_t data[32];
};

struct Signature {
    uint8_t data[64];
};

struct Seckey {
    uint8_t data[32];
};

static inline bool compare_keys(const EventId* a, const EventId* b) {
    auto a_ints = (const uint64_t*)a->data;
    auto b_ints = (const uint64_t*)b->data;
    if (a_ints[0] != b_ints[0] ||
        a_ints[1] != b_ints[1] ||
        a_ints[2] != b_ints[2] ||
        a_ints[3] != b_ints[3]) {
        return false;
    }
    return true;
}

static inline bool compare_keys(const Pubkey* a, const Pubkey* b) {
    auto a_ints = (const uint64_t*)a->data;
    auto b_ints = (const uint64_t*)b->data;
    if (a_ints[0] != b_ints[0] ||
        a_ints[1] != b_ints[1] ||
        a_ints[2] != b_ints[2] ||
        a_ints[3] != b_ints[3]) {
        return false;
    }
    return true;
}

bool get_public_key(const Seckey* seckey, Pubkey* pubkey);
