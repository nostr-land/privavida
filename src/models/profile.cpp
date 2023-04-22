//
//  profile.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-21.
//

#include "profile.hpp"
#include <rapidjson/reader.h>

struct ProfileReader : public rapidjson::BaseReaderHandler<rapidjson::UTF8<>, ProfileReader> {
    enum ReaderState {
        STATE_STARTED,
        STATE_IN_ROOT,
        STATE_AT_NAME,
        STATE_AT_DISPLAY_NAME,
        STATE_AT_PICTURE,
        STATE_AT_WEBSITE,
        STATE_AT_BANNER,
        STATE_AT_NIP05,
        STATE_AT_ABOUT,
        STATE_AT_LUD16,
        STATE_IN_OTHER,
        STATE_ENDED
    };

    ReaderState state = STATE_STARTED;
    int other_depth;
    Profile* result;
    uint32_t current_offset;
    uint8_t* buffer_ptr;

    bool next(ReaderState next_state) {
        state = next_state;
        return true;
    }
    bool done() {
        state = STATE_ENDED;
        return false;
    }
    bool error() {
        return false;
    }
    RelString write_string(const char* str, rapidjson::SizeType length) {
        RelString string;
        string.data.offset = current_offset;
        string.size = length;

        memcpy(buffer_ptr, str, length);
        buffer_ptr[length] = '\0';
        buffer_ptr     += length + 1;
        current_offset += length + 1;

        return string;
    }

    bool Null() {
        if (state == STATE_IN_OTHER) {
            return next(other_depth ? STATE_IN_OTHER : STATE_IN_ROOT);
        } else if (state == STATE_STARTED || state == STATE_IN_ROOT) {
            return error();
        } else {
            return next(STATE_IN_ROOT);
        }
    }
    bool Bool(bool b) {
        if (state == STATE_IN_OTHER) {
            return next(other_depth ? STATE_IN_OTHER : STATE_IN_ROOT);
        } else {
            return error();
        }
    }
    bool Int(int i) {
        if (state == STATE_IN_OTHER) {
            return next(other_depth ? STATE_IN_OTHER : STATE_IN_ROOT);
        } else {
            return next(STATE_IN_ROOT);
        }
    }
    bool Uint(unsigned u) {
        if (state == STATE_IN_OTHER) {
            return next(other_depth ? STATE_IN_OTHER : STATE_IN_ROOT);
        } else {
            return next(STATE_IN_ROOT);
        }
    }
    bool Int64(int64_t i) {
        if (state == STATE_IN_OTHER) {
            return next(other_depth ? STATE_IN_OTHER : STATE_IN_ROOT);
        } else {
            return next(STATE_IN_ROOT);
        }
    }
    bool Uint64(uint64_t u) {
        if (state == STATE_IN_OTHER) {
            return next(other_depth ? STATE_IN_OTHER : STATE_IN_ROOT);
        } else {
            return next(STATE_IN_ROOT);
        }
    }
    bool Double(double d) {
        if (state == STATE_IN_OTHER) {
            return next(other_depth ? STATE_IN_OTHER : STATE_IN_ROOT);
        } else {
            return next(STATE_IN_ROOT);
        }
    }
    bool String(const char* str, rapidjson::SizeType length, bool copy) {
        if (state == STATE_STARTED) {
            return error();
        } else if (state == STATE_IN_ROOT) {
            return error();
        } else if (state == STATE_AT_NAME) {
            result->name = write_string(str, length);
            return next(STATE_IN_ROOT);
        } else if (state == STATE_AT_DISPLAY_NAME) {
            result->display_name = write_string(str, length);
            return next(STATE_IN_ROOT);
        } else if (state == STATE_AT_PICTURE) {
            result->picture = write_string(str, length);
            return next(STATE_IN_ROOT);
        } else if (state == STATE_AT_WEBSITE) {
            result->website = write_string(str, length);
            return next(STATE_IN_ROOT);
        } else if (state == STATE_AT_BANNER) {
            result->banner = write_string(str, length);
            return next(STATE_IN_ROOT);
        } else if (state == STATE_AT_NIP05) {
            result->nip05 = write_string(str, length);
            return next(STATE_IN_ROOT);
        } else if (state == STATE_AT_ABOUT) {
            result->about = write_string(str, length);
            return next(STATE_IN_ROOT);
        } else if (state == STATE_AT_LUD16) {
            result->lud16 = write_string(str, length);
            return next(STATE_IN_ROOT);
        } else if (state == STATE_IN_OTHER) {
            return next(other_depth ? STATE_IN_OTHER : STATE_IN_ROOT);
        } else {
            return error();
        }
    }
    bool StartObject() {
        if (state == STATE_STARTED) {
            return next(STATE_IN_ROOT);
        } else {
            other_depth++;
            return next(STATE_IN_OTHER);
        }
    }
    bool Key(const char* str, rapidjson::SizeType length, bool copy) {
        if (state == STATE_IN_OTHER) return true;
        if (state != STATE_IN_ROOT)  return error();

        other_depth = 0;
        if (strcmp("name", str) == 0) {
            return next(STATE_AT_NAME);
        } else if (strcmp("display_name", str) == 0) {
            return next(STATE_AT_DISPLAY_NAME);
        } else if (strcmp("picture", str) == 0) {
            return next(STATE_AT_PICTURE);
        } else if (strcmp("website", str) == 0) {
            return next(STATE_AT_WEBSITE);
        } else if (strcmp("banner", str) == 0) {
            return next(STATE_AT_BANNER);
        } else if (strcmp("nip05", str) == 0) {
            return next(STATE_AT_NIP05);
        } else if (strcmp("about", str) == 0) {
            return next(STATE_AT_ABOUT);
        } else if (strcmp("lud16", str) == 0) {
            return next(STATE_AT_LUD16);
        } else {
            return next(STATE_IN_OTHER);
        }

        return true;
    }
    bool EndObject(rapidjson::SizeType memberCount) {
        if (state == STATE_IN_OTHER) {
            if (--other_depth <= 0) {
                next(STATE_IN_ROOT);
            }
            return next(STATE_IN_OTHER);
        } else if (state == STATE_IN_ROOT) {
            return done();
        }
        return error();
    }
    bool StartArray() {
        if (state == STATE_STARTED) {
            return error();
        } else {
            return next(STATE_IN_OTHER);
        }
    }
    bool EndArray(rapidjson::SizeType elementCount) {
        return true;
    }
};

static inline size_t max(size_t a, size_t b) {
    return a < b ? b : a;
}

bool parse_profile_data(Profile* profile, const Event* event) {

    auto _base = (void*)profile;
    memset(_base, 0, sizeof(Profile));

    int fixed_size = sizeof(Profile);
    int total_size = fixed_size + event->content.size;

    int data_offset = fixed_size;

    profile->__header__ = (Profile::VERSION << 24) | (uint32_t)total_size;
    profile->pubkey = event->pubkey;
    profile->event_id = event->id;

    ProfileReader handler;
    handler.buffer_ptr = profile->__buffer;
    handler.current_offset = data_offset;
    handler.result = profile;

    auto stack_size = max(event->content.size, 512);
    char stack_memory[stack_size];
    rapidjson::MemoryPoolAllocator<> allocator(stack_memory, stack_size);
    rapidjson::GenericReader<rapidjson::UTF8<>, rapidjson::UTF8<>, rapidjson::MemoryPoolAllocator<>> reader(&allocator);

    rapidjson::StringStream stream(event->content.data.get(event));
    reader.Parse(stream, handler);

    if (reader.HasParseError() && handler.state != ProfileReader::STATE_ENDED) {
        return false;
    }

    return true;
}
