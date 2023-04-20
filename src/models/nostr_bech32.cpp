//
//  nostr_bech32.cpp
//  damus
//
//  Created by Bartholomew Joyce on 2023-04-03.
//

#include "nostr_bech32.hpp"

extern "C" {
#include "c/bech32.h"
}

#include <stdlib.h>
#include <string.h>

enum TLV_Type : uint8_t {
    TLV_SPECIAL = 0,
    TLV_RELAY = 1,
    TLV_AUTHOR = 2,
    TLV_KIND = 3,
};

bool NostrBech32::parse(NostrBech32* mention, uint8_t* buffer, const char* str, int len) {

    const bool SUCCESS = true;
    const bool FAIL = false;

    char prefix[len];
    uint8_t words[len];
    size_t words_len;
    size_t max_input_len = len + 2;

    if (bech32_decode(prefix, words, &words_len, str, max_input_len) == BECH32_ENCODING_NONE) {
        return FAIL;
    }

    size_t data_len = 0;
    if (!bech32_convert_bits(mention->buffer, &data_len, 8, words, words_len, 5, 0)) {
        return FAIL;
    }

    memset(mention, 0, sizeof(NostrBech32));
    mention->buffer = buffer;
    mention->kind = -1;

    // Parse type
    if (strcmp(prefix, "note") == 0) {
        mention->type = NOTE;
    } else if (strcmp(prefix, "npub") == 0) {
        mention->type = NPUB;
    } else if (strcmp(prefix, "nprofile") == 0) {
        mention->type = NPROFILE;
    } else if (strcmp(prefix, "nevent") == 0) {
        mention->type = NEVENT;
    } else if (strcmp(prefix, "nrelay") == 0) {
        mention->type = NRELAY;
    } else if (strcmp(prefix, "naddr") == 0) {
        mention->type = NADDR;
    } else {
        return FAIL;
    }

    // Parse notes and npubs (non-TLV)
    if (mention->type == NOTE || mention->type == NPUB) {
        if (data_len != 32) return FAIL;
        if (mention->type == NOTE) {
            mention->event_id = mention->buffer;
        } else {
            mention->pubkey = mention->buffer;
        }
        return SUCCESS;
    }

    // Parse TLV entities
    const int MAX_VALUES = 16;
    int values_count = 0;
    uint8_t Ts[MAX_VALUES];
    uint8_t Ls[MAX_VALUES];
    uint8_t* Vs[MAX_VALUES];
    for (int i = 0; i < data_len - 1;) {
        if (values_count == MAX_VALUES) return FAIL;

        Ts[values_count] = mention->buffer[i++];
        Ls[values_count] = mention->buffer[i++];
        if (Ls[values_count] > data_len - i) return FAIL;

        Vs[values_count] = &mention->buffer[i];
        i += Ls[values_count];
        ++values_count;
    }

    // Decode and validate all TLV-type entities
    if (mention->type == NPROFILE) {
        for (int i = 0; i < values_count; ++i) {
            if (Ts[i] == TLV_SPECIAL) {
                if (Ls[i] != 32 || mention->pubkey) return FAIL;
                mention->pubkey = Vs[i];
            } else if (Ts[i] == TLV_RELAY) {
                if (mention->relays_count == MAX_RELAYS) return FAIL;
                Vs[i][Ls[i]] = 0;
                mention->relays[mention->relays_count++] = (char*)Vs[i];
            } else {
                return FAIL;
            }
        }
        if (!mention->pubkey) return FAIL;

    } else if (mention->type == NEVENT) {
        for (int i = 0; i < values_count; ++i) {
            if (Ts[i] == TLV_SPECIAL) {
                if (Ls[i] != 32 || mention->event_id) return FAIL;
                mention->event_id = Vs[i];
            } else if (Ts[i] == TLV_RELAY) {
                if (mention->relays_count == MAX_RELAYS) return FAIL;
                Vs[i][Ls[i]] = 0;
                mention->relays[mention->relays_count++] = (char*)Vs[i];
            } else if (Ts[i] == TLV_AUTHOR) {
                if (Ls[i] != 32 || mention->pubkey) return FAIL;
                mention->pubkey = Vs[i];
            } else {
                return FAIL;
            }
        }
        if (!mention->event_id) return FAIL;

    } else if (mention->type == NRELAY) {
        if (values_count != 1 || Ts[0] != TLV_SPECIAL) return FAIL;
        Vs[0][Ls[0]] = 0;
        mention->relays[mention->relays_count++] = (char*)Vs[0];

    } else { // entity.type == NADDR
        for (int i = 0; i < values_count; ++i) {
            if (Ts[i] == TLV_SPECIAL) {
                Vs[i][Ls[i]] = 0;
                mention->identifier = (char*)Vs[i];
            } else if (Ts[i] == TLV_RELAY) {
                if (mention->relays_count == MAX_RELAYS) return FAIL;
                Vs[i][Ls[i]] = 0;
                mention->relays[mention->relays_count++] = (char*)Vs[i];
            } else if (Ts[i] == TLV_AUTHOR) {
                if (Ls[i] != 32 || mention->pubkey) return FAIL;
                mention->pubkey = Vs[i];
            } else if (Ts[i] == TLV_KIND) {
                if (Ls[i] != sizeof(int) || mention->kind != -1) return FAIL;
                mention->kind = *(int*)Vs[i];
            } else {
                return FAIL;
            }
        }
        if (!mention->identifier || mention->kind == -1 || !mention->pubkey) return FAIL;
    }

    return SUCCESS;
}
