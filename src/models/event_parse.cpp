//
//  event_parse.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 18/07/2018.
//

#include "event_parse.hpp"
#include "hex.hpp"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <rapidjson/reader.h>

// This parser is designed to work without doing any heap allocations.
// The way it manages to do this is by building an intermediate
// representation of the JSON data that is gauranteed to be smaller
// than the source string.
// This intermediate representation is a simple kind of TLV (type-length-value)
// binary encoding of a Nostr event.

// There are 8 field types that are encoded in the 4 most-significant-bits
// of the type:
#define TYPE_PART_FIELD       0xF0
#define TYPE_END              0x00
#define TYPE_FIELD_ID         0x10
#define TYPE_FIELD_PUBKEY     0x20
#define TYPE_FIELD_SIG        0x30
#define TYPE_FIELD_KIND       0x40
#define TYPE_FIELD_CONTENT    0x50
#define TYPE_FIELD_CREATED_AT 0x60
#define TYPE_FIELD_TAG        0x70 // Start of a tag
#define TYPE_FIELD_TAG_VAL    0x80 // Start of a tag value

// The length of the TLV entry is encoded either in 19 bits or 32 bits:
#define TYPE_LENGTH_19BIT 0x08 // Length is encoded in 19-bits: 3 LSBs of type field + 2 bytes
#define TYPE_LENGTH_32BIT 0x07 // Length is encoded in 32-bits: 4 bytes following type field
#define TYPE_PART_3LSB    0x07

// The 19-bit type encoding seems kind of weird, but it is necessary to be able
// to encode TLV headers (the type and value part) in no more than 3 bytes to
// guarantee that the TLV is smaller than the JSON input under all circumstances.

static void write_tlv(uint8_t*& buffer_ptr, uint8_t type, uint32_t length, const uint8_t* value) {

    // Write type and length
    if (length < (1 << 19)) {
        *buffer_ptr++ = type | TYPE_LENGTH_19BIT | (uint8_t)(length >> 16);
        *buffer_ptr++ = (uint8_t)((length >> 8) % 256);
        *buffer_ptr++ = (uint8_t)(length % 256);
    } else {
        *buffer_ptr++ = type | TYPE_LENGTH_32BIT;
        *buffer_ptr++ = (uint8_t)(length >> 24);
        *buffer_ptr++ = (uint8_t)((length >> 16) % 256);
        *buffer_ptr++ = (uint8_t)((length >> 8) % 256);
        *buffer_ptr++ = (uint8_t)(length % 256);
    }

    // Write value
    memcpy(buffer_ptr, value, length);
    buffer_ptr += length;
}

static void write_end(uint8_t*& buffer_ptr) {
    *buffer_ptr++ = TYPE_END;
}

#define FLAG_PARSED_ID         0x01
#define FLAG_PARSED_PUBKEY     0x02
#define FLAG_PARSED_SIG        0x04
#define FLAG_PARSED_KIND       0x08
#define FLAG_PARSED_CONTENT    0x10
#define FLAG_PARSED_CREATED_AT 0x20
#define FLAG_PARSED_TAGS       0x40

enum ReaderState {
    STATE_STARTED,
    STATE_IN_ROOT,
    STATE_AT_ID,
    STATE_AT_PUBKEY,
    STATE_AT_SIG,
    STATE_AT_KIND,
    STATE_AT_CREATED_AT,
    STATE_AT_CONTENT,
    STATE_AT_TAGS,
    STATE_IN_TAGS,
    STATE_IN_TAG,
    STATE_IN_OTHER
};

struct ReaderHandler : public rapidjson::BaseReaderHandler<rapidjson::UTF8<>, ReaderHandler> {
    EventParseError err = PARSE_NO_ERR;
    ReaderState state = STATE_STARTED;
    int other_depth;
    uint8_t flags = 0;
    uint8_t* buffer_ptr;
    int num_tags = 0;
    int num_tag_values = 0;
    int text_content_len = 0;
    bool is_first_element = false;

    bool error(EventParseError err) {
        this->err = err;
        return false;
    }
    bool error() {
        switch (state) {
            case STATE_STARTED:       err = PARSE_ERR_INVALID_EVENT;      break;
            case STATE_IN_ROOT:       err = PARSE_ERR_INVALID_EVENT;      break;
            case STATE_AT_ID:         err = PARSE_ERR_INVALID_ID;         break;
            case STATE_AT_PUBKEY:     err = PARSE_ERR_INVALID_PUBKEY;     break;
            case STATE_AT_SIG:        err = PARSE_ERR_INVALID_SIG;        break;
            case STATE_AT_KIND:       err = PARSE_ERR_INVALID_KIND;       break;
            case STATE_AT_CREATED_AT: err = PARSE_ERR_INVALID_CREATED_AT; break;
            case STATE_AT_CONTENT:    err = PARSE_ERR_INVALID_CONTENT;    break;
            case STATE_AT_TAGS:       err = PARSE_ERR_INVALID_TAGS;       break;
            case STATE_IN_TAGS:       err = PARSE_ERR_INVALID_TAGS;       break;
            case STATE_IN_TAG:        err = PARSE_ERR_INVALID_TAGS;       break;
            case STATE_IN_OTHER:      err = PARSE_ERR_INVALID_EVENT;      break;
        }
        return false;
    }

    bool Null() {
        if (state == STATE_IN_OTHER) {
            state = other_depth ? STATE_IN_OTHER : STATE_IN_ROOT;
        } else if (state == STATE_AT_CONTENT || state == STATE_AT_TAGS) {
            state = STATE_IN_ROOT;
        } else {
            return error();
        }
        return true;
    }
    bool Bool(bool b) {
        if (state == STATE_IN_OTHER) {
            state = other_depth ? STATE_IN_OTHER : STATE_IN_ROOT;
            return true;
        }
        return error();
    }
    bool Int(int i) {
        if (i >= 0) return Uint(i);
        if (state == STATE_IN_OTHER) {
            state = other_depth ? STATE_IN_OTHER : STATE_IN_ROOT;
            return true;
        }
        return error();
    }
    bool Uint(unsigned u) {
        if (state == STATE_IN_OTHER) {
            state = other_depth ? STATE_IN_OTHER : STATE_IN_ROOT;
        } else if (state == STATE_AT_KIND) {
            state = STATE_IN_ROOT;
            write_tlv(buffer_ptr, TYPE_FIELD_KIND, 4, (uint8_t*)&u);
        } else if (state == STATE_AT_CREATED_AT) {
            state = STATE_IN_ROOT;
            uint64_t value = u;
            write_tlv(buffer_ptr, TYPE_FIELD_CREATED_AT, 8, (uint8_t*)&value);
        } else {
            return error();
        }
        return true;
    }
    bool Int64(int64_t i) {
        if (i >= 0) return Uint64(i);
        if (state == STATE_IN_OTHER) {
            state = other_depth ? STATE_IN_OTHER : STATE_IN_ROOT;
            return true;
        }
        return error();
    }
    bool Uint64(uint64_t u) {
        if (state == STATE_IN_OTHER) {
            state = other_depth ? STATE_IN_OTHER : STATE_IN_ROOT;
        } else if (state == STATE_AT_CREATED_AT) {
            state = STATE_IN_ROOT;
            write_tlv(buffer_ptr, TYPE_FIELD_CREATED_AT, 8, (uint8_t*)&u);
        } else {
            return error();
        }
        return true;
    }
    bool Double(double d) {
        if (state == STATE_IN_OTHER) {
            state = other_depth ? STATE_IN_OTHER : STATE_IN_ROOT;
            return true;
        }
        return error();
    }
    bool String(const char* str, rapidjson::SizeType length, bool copy) {
        if (state == STATE_IN_OTHER) {
            state = other_depth ? STATE_IN_OTHER : STATE_IN_ROOT;
        } else if (state == STATE_AT_ID) {
            if (length != 2 * sizeof(Event::id)) return error();
            uint8_t bytes[sizeof(Event::id)];
            if (!hex_decode(bytes, str, sizeof(Event::id))) return error();
            write_tlv(buffer_ptr, TYPE_FIELD_ID, length, bytes);
            state = STATE_IN_ROOT;
        } else if (state == STATE_AT_PUBKEY) {
            if (length != 2 * sizeof(Event::pubkey)) return error();
            uint8_t bytes[sizeof(Event::pubkey)];
            if (!hex_decode(bytes, str, sizeof(Event::pubkey))) return error();
            write_tlv(buffer_ptr, TYPE_FIELD_PUBKEY, length, bytes);
            state = STATE_IN_ROOT;
        } else if (state == STATE_AT_SIG) {
            if (length != 2 * sizeof(Event::sig)) return error();
            uint8_t bytes[sizeof(Event::sig)];
            if (!hex_decode(bytes, str, sizeof(Event::sig))) return error();
            write_tlv(buffer_ptr, TYPE_FIELD_SIG, length, bytes);
            state = STATE_IN_ROOT;
        } else if (state == STATE_AT_CONTENT) {
            write_tlv(buffer_ptr, TYPE_FIELD_CONTENT, length, (const uint8_t*)str);
            text_content_len += length + 1;
            state = STATE_IN_ROOT;
        } else if (state == STATE_IN_TAG) {
            uint8_t type = is_first_element ? TYPE_FIELD_TAG : TYPE_FIELD_TAG_VAL;
            write_tlv(buffer_ptr, type, length, (const uint8_t*)str);
            text_content_len += length + 1;
            is_first_element = false;
        } else {
            return error();
        }
        return true;
    }
    bool StartObject() {
        if (state == STATE_STARTED) {
            state = STATE_IN_ROOT;
        } else if (state == STATE_IN_OTHER) {
            other_depth++;
        } else {
            return error();
        }
        return true;
    }
    bool Key(const char* str, rapidjson::SizeType length, bool copy) {
        if (state == STATE_IN_OTHER) return true;
        if (state != STATE_IN_ROOT)  return error();

        if (strcmp("id", str) == 0) {
            if (flags & FLAG_PARSED_ID) return error(PARSE_ERR_DUPLICATE_ID);
            flags |= FLAG_PARSED_ID;
            state = STATE_AT_ID;
        } else if (strcmp("pubkey", str) == 0) {
            if (flags & FLAG_PARSED_PUBKEY) return error(PARSE_ERR_DUPLICATE_PUBKEY);
            flags |= FLAG_PARSED_PUBKEY;
            state = STATE_AT_PUBKEY;
        } else if (strcmp("sig", str) == 0) {
            if (flags & FLAG_PARSED_SIG) return error(PARSE_ERR_DUPLICATE_SIG);
            flags |= FLAG_PARSED_SIG;
            state = STATE_AT_SIG;
        } else if (strcmp("kind", str) == 0) {
            if (flags & FLAG_PARSED_KIND) return error(PARSE_ERR_DUPLICATE_KIND);
            flags |= FLAG_PARSED_KIND;
            state = STATE_AT_KIND;
        } else if (strcmp("created_at", str) == 0) {
            if (flags & FLAG_PARSED_CREATED_AT) return error(PARSE_ERR_DUPLICATE_CREATED_AT);
            flags |= FLAG_PARSED_CREATED_AT;
            state = STATE_AT_CREATED_AT;
        } else if (strcmp("content", str) == 0) {
            if (flags & FLAG_PARSED_CONTENT) return error(PARSE_ERR_DUPLICATE_CONTENT);
            flags |= FLAG_PARSED_CONTENT;
            state = STATE_AT_CONTENT;
        } else if (strcmp("tags", str) == 0) {
            if (flags & FLAG_PARSED_TAGS) return error(PARSE_ERR_DUPLICATE_TAGS);
            flags |= FLAG_PARSED_TAGS;
            state = STATE_AT_TAGS;
        } else {
            state = STATE_IN_OTHER;
            other_depth = 0;
        }

        return true;
    }
    bool EndObject(rapidjson::SizeType memberCount) {
        if (state == STATE_IN_OTHER) {
            if (--other_depth <= 0) {
                state = STATE_IN_ROOT;
            }
            return true;
        } else if (state == STATE_IN_ROOT) {
            write_end(buffer_ptr);
            return true;
        }
        return error();
    }
    bool StartArray() {
        if (state == STATE_IN_OTHER) {
            other_depth++;
        } else if (state == STATE_AT_TAGS) {
            state = STATE_IN_TAGS;
        } else if (state == STATE_IN_TAGS) {
            is_first_element = true;
            state = STATE_IN_TAG;
        } else {
            return error();
        }
        return true;
    }
    bool EndArray(rapidjson::SizeType elementCount) {
        if (state == STATE_IN_OTHER) {
            if (--other_depth <= 0) {
                state = STATE_IN_ROOT;
            }
        } else if (state == STATE_IN_TAG) {
            if (elementCount == 0) return error();
            num_tag_values += elementCount;
            state = STATE_IN_TAGS;
        } else if (state == STATE_IN_TAGS) {
            num_tags += elementCount;
            state = STATE_IN_ROOT;
        } else {
            return error();
        }
        return true;
    }
};

static inline int align_8(int n) {
    return n + (8 - n%8) % 8;
}

EventParseError event_parse(const char* input, size_t input_len, uint8_t* tlv_out, EventParseResult& result) {
    
    ReaderHandler handler;
    handler.buffer_ptr = tlv_out;

    rapidjson::StringStream stream(input);

    char parse_buf[input_len];
    rapidjson::MemoryPoolAllocator<> allocator(parse_buf, input_len);

    rapidjson::GenericReader<rapidjson::UTF8<>, rapidjson::UTF8<>, rapidjson::MemoryPoolAllocator<>> reader(&allocator);
    reader.Parse(stream, handler);

    if (reader.HasParseError()) {
        return handler.err;
    }

    result.event_size = align_8(
        sizeof(Event) +
        align_8(handler.text_content_len) +
        handler.num_tags * sizeof(RelArray<RelArray<RelString>>) +
        handler.num_tag_values * sizeof(RelArray<RelString>)
    );
    result.num_tags = handler.num_tags;
    result.num_tag_values = handler.num_tag_values;
    return PARSE_NO_ERR;
}

void event_create(Event* event, const uint8_t* tlv, const EventParseResult& result) {
    
    auto _base = (void*)event;
    memset(_base, 0, sizeof(Event));

    int fixed_size = sizeof(Event);
    int total_size = result.event_size;
    
    int tags_size = sizeof(RelArray<RelString>) * result.num_tags;
    int vals_size = sizeof(RelString) * result.num_tag_values;

    int data_offset = fixed_size;
    int tags_offset = fixed_size;
    int vals_offset = fixed_size + tags_size;
    int text_offset = fixed_size + tags_size + vals_size;

    event->__header__ = (Event::VERSION << 24) | (uint32_t)total_size;

    event->tags.size = result.num_tags;
    event->tags.data.offset = tags_offset;
    Array<RelArray<RelString>> tags = event->tags.get(_base);

    RelArray<RelString> tag_values;
    tag_values.size = 0;
    tag_values.data.offset = vals_offset;

    int tag_index = 0;
    int tag_value_index = 0;

    const uint8_t* ch = tlv;
    while (*ch != TYPE_END) {
        uint8_t field_type = (*ch & TYPE_PART_FIELD);

        uint32_t len;
        if (*ch & TYPE_LENGTH_19BIT) {
            len = (*ch++ & TYPE_PART_3LSB) << 16;
            len += *ch++ << 8;
            len += *ch++;
        } else {
            ch++;
            len =  *ch++ << 24;
            len += *ch++ << 16;
            len += *ch++ << 8;
            len += *ch++;
        }

        switch (field_type) {
            case TYPE_FIELD_ID: {
                memcpy(event->id, ch, sizeof(event->id));
                break;
            }
            case TYPE_FIELD_PUBKEY: {
                memcpy(event->pubkey, ch, sizeof(event->pubkey));
                break;
            }
            case TYPE_FIELD_SIG: {
                memcpy(event->sig, ch, sizeof(event->sig));
                break;
            }
            case TYPE_FIELD_KIND: {
                event->kind = *(uint32_t*)ch;
                break;
            }
            case TYPE_FIELD_CONTENT: {
                RelString str;
                str.data.offset = text_offset;
                str.size = len;
                text_offset += len + 1;
                
                auto ptr = &str.get(_base, 0);
                memcpy(ptr, ch, len);
                ptr[len] = '\0';

                event->content = str;
                
                break;
            }
            case TYPE_FIELD_CREATED_AT: {
                event->created_at = *(uint64_t*)ch;
                break;
            }
            case TYPE_FIELD_TAG: {
                RelString str;
                str.data.offset = text_offset;
                str.size = len;
                text_offset += len + 1;

                auto ptr = &str.get(_base, 0);
                memcpy(ptr, ch, len);
                ptr[len] = '\0';

                RelArray<RelString>& tag = tags[tag_index];
                tag = tag_values;
                tag.size = 1;
                tag.data += tag_value_index;
                
                tag_values.get(_base, tag_value_index) = str;
                tag_value_index++;
                tag_index++;
                break;
            }
            case TYPE_FIELD_TAG_VAL: {
                RelString str;
                str.data.offset = text_offset;
                str.size = len;
                text_offset += len + 1;

                auto ptr = &str.get(_base, 0);
                memcpy(ptr, ch, len);
                ptr[len] = '\0';

                RelArray<RelString>& tag = tags[tag_index - 1];
                tag.size += 1;
                
                tag_values.get(_base, tag_value_index) = str;
                tag_value_index++;
                break;
            }
        }

        ch += len;
    }
}
