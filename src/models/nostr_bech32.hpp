//
//  nostr_bech32.hpp
//  damus
//
//  Created by Bartholomew Joyce on 2023-04-03.
//

#pragma once

#include <inttypes.h>

#define MAX_RELAYS 10

struct NostrBech32 {
    enum Type {
        NOTE,
        NPUB,
        NPROFILE,
        NEVENT,
        NRELAY,
        NADDR,
    };

    Type type;

    uint8_t* event_id;
    uint8_t* pubkey;
    char* identifier;
    char* relays[MAX_RELAYS];
    int relays_count;
    int kind;

    uint8_t* buffer;

    static bool parse(NostrBech32* mention, uint8_t* buffer, const char* str, int len);
    
};
