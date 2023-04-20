//
//  nostr_entity.hpp
//  damus
//
//  Created by Bartholomew Joyce on 2023-04-03.
//

#pragma once

#include <inttypes.h>
#include "relative.hpp"
#include "event.hpp"

struct NostrEntity {
    enum Type {
        NOTE,
        NPUB,
        NPROFILE,
        NEVENT,
        NRELAY,
        NADDR,
    };

    Type type;
    EventId event_id;
    Pubkey pubkey;
    RelArray<uint8_t> tlv;

    uint8_t  __buffer[];

    static size_t size_of(const NostrEntity* entity) {
        return sizeof(NostrEntity) + entity->tlv.size;
    }

    static int32_t decoded_size(const char* input, uint32_t input_len); // -1 for error
    static bool decode(NostrEntity* entity, const char* input, uint32_t input_len);

    static int32_t encoded_size(const NostrEntity* entity);
    static void encode(const NostrEntity* entity, char* output, uint32_t* output_len);

    static void encode_npub(const Pubkey* pubkey, char* output, uint32_t* output_len);
    static void encode_note(const EventId* event_id, char* output, uint32_t* output_len);

};
