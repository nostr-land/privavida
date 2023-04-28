//
//  event_compose.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-25.
//

#include "event_compose.hpp"
#include "hex.hpp"
#include <string.h>

static inline uint32_t align_8(uint32_t n) {
    return n + (8 - n%8) % 8;
}

const char* TAG_E = "e";
const char* TAG_P = "p";
const char* ETAG_REPLY   = "reply";
const char* ETAG_ROOT    = "root";
const char* ETAG_MENTION = "mention";

static uint32_t get_sizes(const EventDraft* draft, uint32_t* num_tags, uint32_t* num_tag_values, uint32_t* text_content_len) {

    *num_tags = draft->tags_other.size + draft->p_tags.size + draft->e_tags.size;
    *num_tag_values = 0;

    *text_content_len = 0;
    *text_content_len += strlen(draft->content) + 1;
    *text_content_len += (sizeof(TAG_E) + 1 + 2 * sizeof(EventId) + 1) * draft->e_tags.size;
    *text_content_len += (sizeof(TAG_P) + 1 + 2 * sizeof(Pubkey)  + 1) * draft->p_tags.size;
    for (auto& tag : draft->tags_other) {
        for (auto& tag_value : tag) {
            (*num_tag_values)++;
            *text_content_len += strlen(tag_value) + 1;
        }
    }
    for (auto& e_tag : draft->e_tags) {
        if (e_tag.marker == ETag::REPLY) {
            *num_tag_values += 3;
            *text_content_len += strlen(ETAG_REPLY) + 1;
        } else if (e_tag.marker == ETag::ROOT) {
            *num_tag_values += 3;
            *text_content_len += strlen(ETAG_ROOT) + 1;
        } else if (e_tag.marker == ETag::MENTION) {
            *num_tag_values += 3;
            *text_content_len += strlen(ETAG_MENTION) + 1;
        } else {
            *num_tag_values += 2;
        }
    }
    *num_tag_values += 2 * draft->p_tags.size;

    return align_8(
        sizeof(Event) +
        align_8(*text_content_len) +
        *num_tags * sizeof(RelArray<RelArray<RelString>>) +
        *num_tag_values * sizeof(RelArray<RelString>) +
        draft->e_tags.size * sizeof(ETag) +
        draft->p_tags.size * sizeof(PTag)
    );
}

static RelString write_rel_string(void* base, int* text_offset, const char* str, size_t len) {
    RelString rel_string;
    rel_string.data.offset = *text_offset;
    rel_string.size = (uint32_t)len;
    *text_offset += len + 1;

    auto ptr = &rel_string.get(base, 0);
    memcpy(ptr, str, len);
    ptr[len] = '\0';

    return rel_string;
}

uint32_t event_compose_size(const EventDraft* draft) {
    uint32_t num_tags, num_tag_values, text_content_len;
    return get_sizes(draft, &num_tags, &num_tag_values, &text_content_len);
}

void event_compose(Event* event, const EventDraft* draft) {

    uint32_t num_tags, num_tag_values, text_content_len;
    uint32_t event_size = get_sizes(draft, &num_tags, &num_tag_values, &text_content_len);

    memset(event, 0, sizeof(Event));

    int fixed_size = sizeof(Event);
    int total_size = event_size;
    
    int tags_size   = sizeof(RelArray<RelString>) * num_tags;
    int e_tags_size = sizeof(ETag) * draft->e_tags.size;
    int p_tags_size = sizeof(PTag) * draft->p_tags.size;
    int vals_size   = sizeof(RelString) * num_tag_values;

    int data_offset   = fixed_size;
    int tags_offset   = fixed_size;
    int e_tags_offset = fixed_size + tags_size;
    int p_tags_offset = fixed_size + tags_size + e_tags_size;
    int vals_offset   = fixed_size + tags_size + e_tags_size + p_tags_size;
    int text_offset   = fixed_size + tags_size + e_tags_size + p_tags_size + vals_size;

    event->__header__ = (Event::VERSION << 24) | (uint32_t)total_size;

    // Copy over the pubkey, kind & content
    event->pubkey = draft->pubkey;
    event->kind = draft->kind;
    event->content = write_rel_string(event, &text_offset, draft->content, strlen(draft->content));

    // Copy over all tags
    RelString const_e = write_rel_string(event, &text_offset, TAG_E, strlen(TAG_E));
    RelString const_p = write_rel_string(event, &text_offset, TAG_P, strlen(TAG_P));

    event->tags.size = num_tags;
    event->tags.data.offset = tags_offset;
    Array<RelArray<RelString>> tags = event->tags.get(event);

    RelArray<RelString> tag_values_rel;
    tag_values_rel.size = 0;
    tag_values_rel.data.offset = vals_offset;
    Array<RelString> tag_values = tag_values_rel.get(event);

    event->p_tags.size = draft->p_tags.size;
    event->e_tags.size = draft->e_tags.size;
    event->p_tags.data.offset = p_tags_offset;
    event->e_tags.data.offset = e_tags_offset;

    int tag_index = 0;
    int tag_value_index = 0;

    for (auto& tag_draft : draft->tags_other) {
        RelArray<RelString>& tag = tags[tag_index++];
        tag = tag_values_rel;
        tag.size = tag_draft.size;
        tag.data += tag_value_index;

        for (auto& tag_value_draft : tag_draft) {
            tag_values[tag_value_index++] = write_rel_string(event, &text_offset, tag_value_draft, strlen(tag_value_draft));
        }
    }
    for (int i = 0; i < draft->p_tags.size; ++i) {
        PTag p_tag = draft->p_tags[i];
        p_tag.index = tag_index;
        event->p_tags.get(event, i) = p_tag;

        RelArray<RelString>& tag = tags[tag_index++];
        tag = tag_values_rel;
        tag.size = 2;
        tag.data += tag_value_index;

        char hex_string[2 * sizeof(Pubkey)];
        hex_encode(hex_string, p_tag.pubkey.data, sizeof(Pubkey));

        tag_values[tag_value_index++] = const_p;
        tag_values[tag_value_index++] = write_rel_string(event, &text_offset, hex_string, sizeof(hex_string));
    }
    for (int i = 0; i < draft->e_tags.size; ++i) {
        ETag e_tag = draft->e_tags[i];
        e_tag.index = tag_index;
        event->e_tags.get(event, i) = e_tag;

        RelArray<RelString>& tag = tags[tag_index++];
        tag = tag_values_rel;
        tag.size = 2;
        tag.data += tag_value_index;

        char hex_string[2 * sizeof(EventId)];
        hex_encode(hex_string, e_tag.event_id.data, sizeof(EventId));

        tag_values[tag_value_index++] = const_p;
        tag_values[tag_value_index++] = write_rel_string(event, &text_offset, hex_string, sizeof(hex_string));

        if (e_tag.marker == ETag::REPLY) {
            tag.size++;
            tag_values[tag_value_index++] = write_rel_string(event, &text_offset, ETAG_REPLY, sizeof(ETAG_REPLY));
        } else if (e_tag.marker == ETag::ROOT) {
            tag.size++;
            tag_values[tag_value_index++] = write_rel_string(event, &text_offset, ETAG_ROOT, sizeof(ETAG_ROOT));
        } else if (e_tag.marker == ETag::MENTION) {
            tag.size++;
            tag_values[tag_value_index++] = write_rel_string(event, &text_offset, ETAG_MENTION, sizeof(ETAG_MENTION));
        }
    }

}
