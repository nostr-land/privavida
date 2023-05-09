//
//  event_builder.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-05-09.
//

#pragma once
#include "event.hpp"
#include "hex.hpp"
#include "../utils/stackbuffer.hpp"
#include "relay_info.hpp"

// EventBuilder allows for Event structs to be created in a simple declarative way.
// You pass in a StackBuffer that should house the Event and subsequently chain
// together multiple calls to set the state of the Event.
// Finally you make a call to finish() to get back the finished Event.

// Example kind 4 usage:
//
//      StackBufferFixed<1024> stack_buffer;
//      Event* event = EventBuilder(&stack_buffer)
//          .pubkey(&my_pubkey)
//          .kind(4)
//          .content(ciphertext)
//          .p_tag(&counterparty)
//          .finish();
//

// Example kind 1 usage:
//
//      StackBufferFixed<1024> stack_buffer;
//      int mention_idx;
//      EventBuilder builder(&stack_buffer)
//          .pubkey(&my_pubkey)
//          .kind(1)
//          .p_tag(&root_event->pubkey)
//          .p_tag(&reply_event->pubkey)
//          .e_tag(&root_event->id,  ETag::ROOT)
//          .e_tag(&reply_event->id, ETag::REPLY)
//          .p_tag(&mentioned_pubkey, &mention_idx);
//      
//      char content[64];
//      sprintf(content, "Have you seen #[%d]'s work relating to this?", mention_idx);
//
//      Event* event = builder.content(content).finish();
//

struct EventBuilder {
    
    struct SizeAndOffset {
        uint32_t size;
        uint32_t offset;
        SizeAndOffset() = default;
        SizeAndOffset(uint32_t size, uint32_t offset) : size(size), offset(offset) {}
    };

    StackBuffer* buffer;
    uint32_t buffer_used;

    // We build the tags outside of the Event
    // and we copy them over during finish()
    StackArrayFixed<SizeAndOffset, 8>  tags;
    StackArrayFixed<SizeAndOffset, 16> tag_values;
    StackArrayFixed<char, 256>         tag_content;
    StackArrayFixed<ETag, 8>           e_tags;
    StackArrayFixed<PTag, 8>           p_tags;

    EventBuilder(StackBuffer* buffer_) {
        buffer = buffer_;
        buffer_used = sizeof(Event);
        buffer->reserve(buffer_used);

        auto event = get();
        memset(event, 0, sizeof(Event));
    }

    EventBuilder& id(const EventId* id) {
        get()->id = *id;
        return *this;
    }
    EventBuilder& pubkey(const Pubkey* pubkey) {
        get()->pubkey = *pubkey;
        return *this;
    }
    EventBuilder& kind(uint32_t kind) {
        get()->kind = kind;
        return *this;
    }
    EventBuilder& content(const char* content) {
        get()->content = copy_string(content, (uint32_t)strlen(content));
        return *this;
    }
    EventBuilder& content(const char* content, uint32_t len) {
        get()->content = copy_string(content, len);
        return *this;
    }
    EventBuilder& created_at(uint64_t created_at) {
        get()->created_at = created_at;
        return *this;
    }
    EventBuilder& sig(const Signature* sig) {
        get()->sig = *sig;
        return *this;
    }
    EventBuilder& sent_by_client(bool sent_by_client) {
        get()->sent_by_client = sent_by_client;
        return *this;
    }
    EventBuilder& tag(const Array<const char*>* tag) {
        auto tag_index = (uint32_t)tags.size;
        create_tag(tag);

        if (tag->size >= 2) {
            parse_e_or_p_tag(tag, tag_index);
        }

        return *this;
    }
    EventBuilder& e_tag(const EventId* event_id, ETag::Marker marker = ETag::NO_MARKER, int* index = NULL) {

        ETag e_tag;
        e_tag.index = tags.size;
        e_tag.event_id = *event_id;
        e_tag.marker = marker;
        e_tags.push_back(e_tag);
        if (index) {
            *index = e_tag.index;
        }

        char hex_buffer[sizeof(EventId) * 2 + 1];
        hex_encode(hex_buffer, e_tag.event_id.data, sizeof(EventId));
        hex_buffer[sizeof(EventId) * 2] = '\0';

        const char* tag_values[4];
        uint32_t count = 0;

        tag_values[count++] = "e";
        tag_values[count++] = hex_buffer;
        tag_values[count++] = ""; // <relay-url> (not implemented)
        switch (e_tag.marker) {
            case ETag::NO_MARKER: break;
            case ETag::REPLY:     tag_values[count++] = "reply";   break;
            case ETag::ROOT:      tag_values[count++] = "root";    break;
            case ETag::MENTION:   tag_values[count++] = "mention"; break;
        }

        Array<const char*> tag_out(count, tag_values);
        create_tag(&tag_out);

        return *this;
    }
    EventBuilder& p_tag(const Pubkey* pubkey, int* index = NULL) {

        PTag p_tag;
        p_tag.index = tags.size;
        p_tag.pubkey = *pubkey;
        p_tags.push_back(p_tag);
        if (index) {
            *index = p_tag.index;
        }

        char hex_buffer[sizeof(Pubkey) * 2 + 1];
        hex_encode(hex_buffer, p_tag.pubkey.data, sizeof(Pubkey));
        hex_buffer[sizeof(Pubkey) * 2] = '\0';

        const char* tag_values[2];
        uint32_t count = 0;

        tag_values[count++] = "p";
        tag_values[count++] = hex_buffer;

        Array<const char*> tag_out(count, tag_values);
        create_tag(&tag_out);

        return *this;
    }
    Event* finish() {

        // Copy the e_tag and p_tag arrays into the event
        Array<ETag> e_tags_array((uint32_t)e_tags.size, e_tags.begin());
        Array<PTag> p_tags_array((uint32_t)p_tags.size, p_tags.begin());
        get()->e_tags = copy_array(&e_tags_array);
        get()->p_tags = copy_array(&p_tags_array);

        // Copy the tags into the event
        RelString tag_values_out[tag_values.size];
        for (int i = 0; i < tag_values.size; ++i) {
            tag_values_out[i] = copy_string(&tag_content[tag_values[i].offset], tag_values[i].size);
        }

        RelArray<RelString> tags_out[tags.size];
        for (int i = 0; i < tags.size; ++i) {
            Array<RelString> tag_values_array(tags[i].size, &tag_values_out[tags[i].offset]);
            tags_out[i] = copy_array(&tag_values_array);
        }

        Array<RelArray<RelString>> tags_out_array((uint32_t)tags.size, tags_out);
        get()->tags = copy_array(&tags_out_array);

        // Allocate space for receipt info metadata
        {
            constexpr int NUM_SPACE = 16;
            ReceiptInfo receipt_info[NUM_SPACE];
            memset(receipt_info, 0, NUM_SPACE * sizeof(ReceiptInfo));
            Array<ReceiptInfo> receipt_info_array(NUM_SPACE, receipt_info);
            auto receipt_info_rel_array = copy_array(&receipt_info_array);
            get()->receipt_info.size = 0;
            get()->receipt_info.space = NUM_SPACE;
            get()->receipt_info.data = receipt_info_rel_array.data;
        }

        // Allocate space for publish info metadata
        if (get()->sent_by_client) {
            constexpr int NUM_SPACE = 16;
            PublishInfo publish_info[NUM_SPACE];
            memset(publish_info, 0, NUM_SPACE * sizeof(PublishInfo));
            Array<PublishInfo> publish_info_array(NUM_SPACE, publish_info);
            auto publish_info_rel_array = copy_array(&publish_info_array);
            get()->publish_info.size = 0;
            get()->publish_info.space = NUM_SPACE;
            get()->publish_info.data = publish_info_rel_array.data;
        }

        Event::set_size(get(), buffer_used);
        return get();
    }

private:
    template <typename T>
    RelArray<T> copy_array(const Array<T>* array) {
        size_t size = sizeof(T) * array->size;
        buffer->reserve(buffer_used + size);

        memcpy((int8_t*)buffer->data + buffer_used, array->data, size);

        RelArray<T> rel_array;
        rel_array.size = array->size;
        rel_array.data.offset = buffer_used;

        buffer_used += size;

        return rel_array;
    }

    RelString copy_string(const char* string, uint32_t len) {
        size_t size = len + 1;
        buffer->reserve(buffer_used + size);

        auto ptr = (int8_t*)buffer->data + buffer_used;
        memcpy(ptr, string, len);
        ptr[len] = '\0';

        RelString rel_string;
        rel_string.size = len;
        rel_string.data.offset = buffer_used;

        buffer_used += size;

        return rel_string;
    }

    void create_tag(const Array<const char*>* tag) {
        tags.push_back(SizeAndOffset(tag->size, (uint32_t)tag_values.size));

        tag_values.reserve(tag_values.size + tag->size);
        for (auto& tag_value : *tag) {
            auto len  = (uint32_t)strlen(tag_value);
            auto size = len + 1;

            // Copy the tag value content into tag_content
            tag_content.reserve(tag_content.size + size);
            memcpy(tag_content.end(), tag_value, size);

            tag_values.push_back(SizeAndOffset(len, (uint32_t)tag_content.size));
            tag_content.size += size;
        }
    }

    void parse_e_or_p_tag(const Array<const char*>* tag, uint32_t tag_index) {

        if (strcmp((*tag)[0], "e") == 0) {

            // Expecting:
            // ["e", <event-id>] or
            // ["e", <event-id>, <relay-url>] or
            // ["e", <event-id>, <relay-url>, <marker>]

            ETag e_tag;
            e_tag.index = tag_index;
            e_tag.marker = ETag::NO_MARKER;

            if (strlen((*tag)[1]) != 2 * sizeof(EventId)) return;
            if (!hex_decode(e_tag.event_id.data, (*tag)[1], sizeof(EventId))) return;

            if (tag->size == 4) {
                auto marker = (*tag)[3];
                if (strcmp(marker, "reply")) {
                    e_tag.marker = ETag::REPLY;
                } else if (strcmp(marker, "root")) {
                    e_tag.marker = ETag::ROOT;
                } else if (strcmp(marker, "mention")) {
                    e_tag.marker = ETag::MENTION;
                }
            }

            e_tags.push_back(e_tag);

        } else if (strcmp((*tag)[0], "p") == 0) {

            // Expecting:
            // ["p", <pubkey>]

            PTag p_tag;
            p_tag.index = tag_index;

            if (strlen((*tag)[1]) != 2 * sizeof(Pubkey)) return;
            if (!hex_decode(p_tag.pubkey.data, (*tag)[1], sizeof(Pubkey))) return;

            p_tags.push_back(p_tag);

        }

    }

    Event* get() {
        return (Event*)buffer->data;
    }
};
