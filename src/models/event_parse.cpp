//
//  event_parse.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-10.
//

#include "event_parse.hpp"
#include "event_builder.hpp"
#include "hex.hpp"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <rapidjson/reader.h>

#define FLAG_HAS_ID         0x01
#define FLAG_HAS_PUBKEY     0x02
#define FLAG_HAS_SIG        0x04
#define FLAG_HAS_KIND       0x08
#define FLAG_HAS_CONTENT    0x10
#define FLAG_HAS_CREATED_AT 0x20
#define FLAG_HAS_TAGS       0x40

struct EventIntermediateRepresentation {
    uint8_t   flags;
    EventId   id;
    Pubkey    pubkey;
    Signature sig;
    uint32_t  kind;
    uint64_t  created_at;
    uint8_t*  content_and_tags;
    uint8_t*  content_and_tags_end;
    int       num_tags;
    int       num_tag_values;
    int       num_e_tags;
    int       num_p_tags;
};

// On the first pass the content and tags are encoded
// together into one uint8_t array. It's broken into
// parts, there's a one byte TYPE followed by a
// NULL-terminated string for the data
enum TLV_Type : uint8_t {
    TYPE_END        = 0x00,
    TYPE_CONTENT    = 0x01,
    TYPE_TAG        = 0x02, // Start of a tag
    TYPE_TAG_VALUE  = 0x03, // Start of a tag value
};

// The EventReader struct implements rapidjson's BaseReaderHandler
// class. It receives tokens as they are parsed by rapidjson and
// turns them into an EventIntermediateRepresentation struct
struct EventReader : public rapidjson::BaseReaderHandler<rapidjson::UTF8<>, EventReader> {
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
        STATE_IN_OTHER,
        STATE_ENDED
    };

    EventIntermediateRepresentation event;
    ParseError err = PARSE_NO_ERR;
    ReaderState state = STATE_STARTED;
    int other_depth;
    bool is_first_element = false;

    bool error(ParseError err) {
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
            case STATE_ENDED:         err = PARSE_ERR_INVALID_EVENT;      break;
        }
        return false;
    }
    bool done() {
        state = STATE_ENDED;
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
            event.kind = u;
        } else if (state == STATE_AT_CREATED_AT) {
            state = STATE_IN_ROOT;
            event.created_at = u;
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
            event.created_at = u;
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
            if (length != 2 * sizeof(EventId)) return error();
            if (!hex_decode(event.id.data, str, sizeof(EventId))) return error();
            state = STATE_IN_ROOT;
        } else if (state == STATE_AT_PUBKEY) {
            if (length != 2 * sizeof(Pubkey)) return error();
            if (!hex_decode(event.pubkey.data, str, sizeof(Pubkey))) return error();
            state = STATE_IN_ROOT;
        } else if (state == STATE_AT_SIG) {
            if (length != 2 * sizeof(Signature)) return error();
            if (!hex_decode(event.sig.data, str, sizeof(Signature))) return error();
            state = STATE_IN_ROOT;
        } else if (state == STATE_AT_CONTENT) {
            write_content_or_tag(TYPE_CONTENT, length, (const uint8_t*)str);
            state = STATE_IN_ROOT;
        } else if (state == STATE_IN_TAG) {
            uint8_t type = is_first_element ? TYPE_TAG : TYPE_TAG_VALUE;
            if (is_first_element && length == 1) {
                if (str[0] == 'e') event.num_e_tags++;
                if (str[0] == 'p') event.num_p_tags++;
            }
            write_content_or_tag(type, length, (const uint8_t*)str);
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
            if (event.flags & FLAG_HAS_ID) return error(PARSE_ERR_DUPLICATE_ID);
            event.flags |= FLAG_HAS_ID;
            state = STATE_AT_ID;
        } else if (strcmp("pubkey", str) == 0) {
            if (event.flags & FLAG_HAS_PUBKEY) return error(PARSE_ERR_DUPLICATE_PUBKEY);
            event.flags |= FLAG_HAS_PUBKEY;
            state = STATE_AT_PUBKEY;
        } else if (strcmp("sig", str) == 0) {
            if (event.flags & FLAG_HAS_SIG) return error(PARSE_ERR_DUPLICATE_SIG);
            event.flags |= FLAG_HAS_SIG;
            state = STATE_AT_SIG;
        } else if (strcmp("kind", str) == 0) {
            if (event.flags & FLAG_HAS_KIND) return error(PARSE_ERR_DUPLICATE_KIND);
            event.flags |= FLAG_HAS_KIND;
            state = STATE_AT_KIND;
        } else if (strcmp("created_at", str) == 0) {
            if (event.flags & FLAG_HAS_CREATED_AT) return error(PARSE_ERR_DUPLICATE_CREATED_AT);
            event.flags |= FLAG_HAS_CREATED_AT;
            state = STATE_AT_CREATED_AT;
        } else if (strcmp("content", str) == 0) {
            if (event.flags & FLAG_HAS_CONTENT) return error(PARSE_ERR_DUPLICATE_CONTENT);
            event.flags |= FLAG_HAS_CONTENT;
            state = STATE_AT_CONTENT;
        } else if (strcmp("tags", str) == 0) {
            if (event.flags & FLAG_HAS_TAGS) return error(PARSE_ERR_DUPLICATE_TAGS);
            event.flags |= FLAG_HAS_TAGS;
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
            return done();
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
            event.num_tag_values += elementCount;
            state = STATE_IN_TAGS;
        } else if (state == STATE_IN_TAGS) {
            event.num_tags += elementCount;
            state = STATE_IN_ROOT;
        } else {
            return error();
        }
        return true;
    }

    void write_content_or_tag(uint8_t type, uint32_t length, const uint8_t* value) {
        auto& ptr = event.content_and_tags_end;
        *ptr++ = type;
        memcpy(ptr, value, length);
        ptr += length;
        *ptr++ = 0; // NULL terminate
    }
};

static inline int align_8(int n) {
    return n + (8 - n%8) % 8;
}

static inline size_t max(size_t a, size_t b) {
    return a < b ? b : a;
}

ParseError event_parse(const char* input, size_t input_len, StackBuffer* stack_buffer, Event** event_out) {

    uint8_t content_and_tags_buffer[input_len];
    
    EventReader handler;
    handler.event = { 0 };
    handler.event.content_and_tags     = content_and_tags_buffer;
    handler.event.content_and_tags_end = content_and_tags_buffer;

    auto stack_size = max(input_len, 512);
    char stack_memory[stack_size];
    rapidjson::MemoryPoolAllocator<> allocator(stack_memory, stack_size);
    rapidjson::GenericReader<rapidjson::UTF8<>, rapidjson::UTF8<>, rapidjson::MemoryPoolAllocator<>> reader(&allocator);

    rapidjson::StringStream stream(input);
    reader.Parse(stream, handler);

    if (reader.HasParseError() && handler.state != EventReader::STATE_ENDED) {
        return handler.err;
    }
    
    EventBuilder builder(stack_buffer);

    if (handler.event.flags & FLAG_HAS_ID) {
        builder.id(&handler.event.id);
    } else {
        return PARSE_ERR_MISSING_ID;
    }

    if (handler.event.flags & FLAG_HAS_PUBKEY) {
        builder.pubkey(&handler.event.pubkey);
    } else {
        return PARSE_ERR_MISSING_PUBKEY;
    }
    
    if (handler.event.flags & FLAG_HAS_SIG) {
        builder.sig(&handler.event.sig);
    } else {
        return PARSE_ERR_MISSING_SIG;
    }
    
    if (handler.event.flags & FLAG_HAS_KIND) {
        builder.kind(handler.event.kind);
    } else {
        return PARSE_ERR_MISSING_KIND;
    }
    
    if (handler.event.flags & FLAG_HAS_CREATED_AT) {
        builder.created_at(handler.event.created_at);
    } else {
        return PARSE_ERR_MISSING_CREATED_AT;
    }

    // As we know the total sizes of all tags & tag values we can make sure the EventBuilder won't allocate
    // any extra data on the heap by overriding the builder's internal StackArrays
    typedef EventBuilder::SizeAndOffset Pair;
    Pair stack_tags[handler.event.num_tags];
    Pair stack_tag_values[handler.event.num_tag_values];
    char stack_tag_content[input_len];
    ETag stack_e_tags[handler.event.num_e_tags];
    PTag stack_p_tags[handler.event.num_p_tags];
    builder.tags.buffer.reset(stack_tags, handler.event.num_tags * sizeof(Pair));
    builder.tag_values.buffer.reset(stack_tag_values, handler.event.num_tag_values * sizeof(Pair));
    builder.tag_content.buffer.reset(stack_tag_content, input_len);
    builder.e_tags.buffer.reset(stack_e_tags, handler.event.num_e_tags * sizeof(ETag));
    builder.p_tags.buffer.reset(stack_p_tags, handler.event.num_p_tags * sizeof(PTag));

    const char* tag_values[handler.event.num_tag_values];
    int tag_values_count = 0;

    // Pull out the content and tags data
    const uint8_t* ch = handler.event.content_and_tags;
    while (ch < handler.event.content_and_tags_end) {
        uint8_t field_type = *ch++;
        auto str = (const char*)ch;
        auto len = (uint32_t)strlen(str);
        ch += len + 1;

        switch (field_type) {
            case TYPE_CONTENT: {
                builder.content(str, len);
                break;
            }
            case TYPE_TAG: {
                if (tag_values_count) {
                    Array<const char*> tag(tag_values_count, tag_values);
                    builder.tag(&tag);
                    tag_values_count = 0;
                }
                // @IMPORTANT: fall through to TYPE_TAG_VALUE case!
            }
            case TYPE_TAG_VALUE: {
                tag_values[tag_values_count++] = str;
                break;
            }
        }
    }
    
    if (tag_values_count) {
        Array<const char*> tag(tag_values_count, tag_values);
        builder.tag(&tag);
        tag_values_count = 0;
    }

    *event_out = builder.finish();
    return PARSE_NO_ERR;
}
