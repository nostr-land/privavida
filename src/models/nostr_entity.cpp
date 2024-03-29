//
//  nostr_entity.cpp
//  damus
//
//  Created by Bartholomew Joyce on 2023-04-03.
//

#include "nostr_entity.hpp"

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
    TLV_SIG = 4,
    TLV_TYPE_COUNT = 5,
};

int32_t NostrEntity::decoded_size(const char* input, uint32_t input_len) {
    
    NostrEntity::Type type;
    if (strncmp(input, "note", strlen("note")) == 0) {
        type = NOTE;
    } else if (strncmp(input, "npub", strlen("npub")) == 0) {
        type = NPUB;
    } else if (strncmp(input, "nsec", strlen("nsec")) == 0) {
        type = NSEC;
    } else if (strncmp(input, "nprofile", strlen("nprofile")) == 0) {
        type = NPROFILE;
    } else if (strncmp(input, "nevent", strlen("nevent")) == 0) {
        type = NEVENT;
    } else if (strncmp(input, "nrelay", strlen("nrelay")) == 0) {
        type = NRELAY;
    } else if (strncmp(input, "naddr", strlen("naddr")) == 0) {
        type = NADDR;
    } else if (strncmp(input, "ninvite", strlen("ninvite")) == 0) {
        type = NINVITE;
    } else {
        return -1;
    }

    if (type == NOTE || type == NPUB || type == NSEC) {
        return sizeof(NostrEntity);
    } else {
        return sizeof(NostrEntity) + input_len;
    }

}

template <typename T>
static inline RelPointer<T> make_rel_pointer(uint8_t* buffer, uint8_t* ptr) {
    uint32_t offset = (uint32_t)(ptr - (uint8_t*)buffer) + sizeof(NostrEntity);
    return RelPointer<T>(offset);
}

bool NostrEntity::decode(NostrEntity* entity, const char* input, uint32_t input_len) {

    const bool SUCCESS = true;
    const bool FAIL = false;

    char prefix[input_len];
    uint8_t words[input_len];
    size_t words_len;
    size_t max_input_len = input_len + 2;

    if (bech32_decode(prefix, words, &words_len, input, max_input_len) == BECH32_ENCODING_NONE) {
        return FAIL;
    }

    size_t data_len = 0;
    uint8_t buffer[words_len];
    if (!bech32_convert_bits(buffer, &data_len, 8, words, words_len, 5, 0)) {
        return FAIL;
    }

    memset(entity, 0, sizeof(NostrEntity));

    // Parse type
    if (strcmp(prefix, "note") == 0) {
        entity->type = NOTE;
    } else if (strcmp(prefix, "npub") == 0) {
        entity->type = NPUB;
    } else if (strcmp(prefix, "nsec") == 0) {
        entity->type = NSEC;
    } else if (strcmp(prefix, "nprofile") == 0) {
        entity->type = NPROFILE;
    } else if (strcmp(prefix, "nevent") == 0) {
        entity->type = NEVENT;
    } else if (strcmp(prefix, "nrelay") == 0) {
        entity->type = NRELAY;
    } else if (strcmp(prefix, "naddr") == 0) {
        entity->type = NADDR;
    } else if (strcmp(prefix, "ninvite") == 0) {
        entity->type = NINVITE;
    } else {
        return FAIL;
    }

    // Parse notes, npubs and nsecs (non-TLV)
    if (entity->type == NOTE) {
        if (data_len != sizeof(EventId)) return FAIL;
        memcpy(entity->event_id.data, buffer, sizeof(EventId));
        return SUCCESS;

    } else if (entity->type == NPUB) {
        if (data_len != sizeof(Pubkey)) return FAIL;
        memcpy(entity->pubkey.data, buffer, sizeof(Pubkey));
        return SUCCESS;

    } else if (entity->type == NSEC) {
        if (data_len != sizeof(Seckey)) return FAIL;
        memcpy(entity->seckey.data, buffer, sizeof(Seckey));
        return SUCCESS;
    }

    // Copy TLV array into entity
    entity->tlv.data.offset = (uint32_t)((uint8_t*)&entity->__buffer - (uint8_t*)entity);
    entity->tlv.size = (uint32_t)data_len;
    memcpy(entity->tlv.data.get(entity), buffer, data_len);

    // Validate TLV data
    int type_counts[TLV_TYPE_COUNT];
    int type_length[TLV_TYPE_COUNT];
    int type_value_idx[TLV_TYPE_COUNT];
    memset(type_counts, 0, sizeof(type_counts));

    for (int i = 0; i < data_len - 1;) {
        uint8_t T = buffer[i++];
        if (T >= TLV_TYPE_COUNT) return FAIL;
        type_counts[T]++;

        uint8_t L = buffer[i++];
        if (L > data_len - i) return FAIL;
        type_length[T] = L;

        type_value_idx[T] = i;
        i += L;
    }

    // Validate all TLV-type entities (and pull out pubkeys and event ids)
    if (entity->type == NPROFILE) {
        if (type_counts[TLV_SPECIAL] != 1 ||
            type_counts[TLV_AUTHOR]  != 0 ||
            type_counts[TLV_KIND]    != 0 ||
            type_counts[TLV_SIG]     != 0 ||
            type_length[TLV_SPECIAL] != sizeof(Pubkey)) {
            return FAIL;
        }

        memcpy(entity->pubkey.data, &buffer[type_value_idx[TLV_SPECIAL]], sizeof(Pubkey));
        return SUCCESS;

    } else if (entity->type == NEVENT) {
        if (type_counts[TLV_SPECIAL] != 1 ||
            type_counts[TLV_AUTHOR]  >  1 ||
            type_counts[TLV_KIND]    >  1 ||
            type_counts[TLV_SIG]     != 0 ||
            type_length[TLV_SPECIAL] != sizeof(EventId)) {
            return FAIL;
        }

        memcpy(entity->event_id.data, &buffer[type_value_idx[TLV_SPECIAL]], sizeof(EventId));
        return SUCCESS;

    } else if (entity->type == NRELAY) {
        if (type_counts[TLV_SPECIAL] != 1 ||
            type_counts[TLV_RELAY]   != 0 ||
            type_counts[TLV_AUTHOR]  != 0 ||
            type_counts[TLV_KIND]    != 0 ||
            type_counts[TLV_SIG]     != 0) {
            return FAIL;
        }
        return SUCCESS;

    } else if (entity->type == NADDR) {
        if (type_counts[TLV_SPECIAL] != 1 ||
            type_counts[TLV_AUTHOR]  >  1 ||
            type_counts[TLV_KIND]    >  1 ||
            type_counts[TLV_SIG]     != 0 ||
            type_length[TLV_AUTHOR]  != sizeof(Pubkey)) {
            return FAIL;
        }

        memcpy(entity->pubkey.data, &buffer[type_value_idx[TLV_AUTHOR]], sizeof(Pubkey));
        return SUCCESS;
    } else { // entity.type == NINVITE
        if (type_counts[TLV_SPECIAL] != 1 ||
            type_counts[TLV_AUTHOR]  != 1 ||
            type_counts[TLV_KIND]    != 0 ||
            type_counts[TLV_SIG]     != 1 ||
            type_length[TLV_SPECIAL] != sizeof(Pubkey) ||
            type_length[TLV_AUTHOR]  != sizeof(Pubkey) ||
            type_length[TLV_SIG]     != sizeof(Signature)) {
            return FAIL;
        }

        memcpy(entity->pubkey.data, &buffer[type_value_idx[TLV_AUTHOR]], sizeof(Pubkey));
        entity->invite_conversation_pubkey = make_rel_pointer<Pubkey>(&buffer[0], &buffer[type_value_idx[TLV_SPECIAL]]);
        entity->invite_signature = make_rel_pointer<Signature>(&buffer[0], &buffer[type_value_idx[TLV_SIG]]);
        return SUCCESS;
    }
}

int32_t NostrEntity::encoded_size(const NostrEntity* entity) {
    return -1;
}

void NostrEntity::encode(const NostrEntity* entity, char* output, uint32_t* output_len) {
    
    const char* prefix;
    const uint8_t* data;
    size_t data_len;
    
    switch (entity->type) {
        case NOTE:
            prefix = "note";
            data = entity->event_id.data;
            data_len = sizeof(EventId);
            break;
        case NPUB:
            prefix = "npub";
            data = entity->pubkey.data;
            data_len = sizeof(Pubkey);
            break;
        case NSEC:
            prefix = "nsec";
            data = entity->seckey.data;
            data_len = sizeof(Seckey);
            break;
        case NPROFILE:
            prefix = "nprofile";
            data = entity->tlv.data.get(entity);
            data_len = entity->tlv.size;
            break;
        case NEVENT:
            prefix = "nevent";
            data = entity->tlv.data.get(entity);
            data_len = entity->tlv.size;
            break;
        case NRELAY:
            prefix = "nrelay";
            data = entity->tlv.data.get(entity);
            data_len = entity->tlv.size;
            break;
        case NADDR:
            prefix = "naddr";
            data = entity->tlv.data.get(entity);
            data_len = entity->tlv.size;
            break;
        case NINVITE:
            prefix = "ninvite";
            data = entity->tlv.data.get(entity);
            data_len = entity->tlv.size;
            break;
    }

    size_t words_len = 0;
    uint8_t words[data_len * 2];
    if (!bech32_convert_bits(words, &words_len, 5, data, data_len, 8, 1)) {
        return;
    }

    bech32_encode(output, prefix, words, words_len, 1024, BECH32_ENCODING_BECH32);
    if (output_len) {
        *output_len = (uint32_t)strlen(output);
    }
}

void NostrEntity::encode_npub(const Pubkey* pubkey, char* output, uint32_t* output_len) {
    NostrEntity entity;
    entity.type = NPUB;
    memcpy(entity.pubkey.data, pubkey, sizeof(Pubkey));
    encode(&entity, output, output_len);
}

void NostrEntity::encode_note(const EventId* event_id, char* output, uint32_t* output_len) {
    NostrEntity entity;
    entity.type = NOTE;
    memcpy(entity.event_id.data, event_id, sizeof(Pubkey));
    encode(&entity, output, output_len);
}
