//
//  relay_message_parse.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 18/07/2018.
//

#include "relay_message_parse.hpp"
#include "hex.hpp"

#include <string.h>
#include <stdlib.h>
#include <rapidjson/reader.h>

static inline size_t max(size_t a, size_t b) {
    return a < b ? b : a;
}


// Message formats
// ["AUTH", <challenge-string>]
// ["COUNT", <subscription_id>, {"count": <integer>}]
// ["EOSE", <subscription_id>]
// ["EVENT", <subscription_id>, <event JSON>]
// ["NOTICE", <message>]
// ["OK", <event_id>, <true|false>, <message>]


struct RelayMessageReader : public rapidjson::BaseReaderHandler<rapidjson::UTF8<>, RelayMessageReader> {
    enum ReaderState {
        STATE_STARTED,
        STATE_AT_MESSAGE_TYPE,
        STATE_AT_AUTH_CHALLENGE_STRING,
        STATE_AT_COUNT_SUBSCRIPTION_ID,
        STATE_AT_COUNT_OBJECT,
        STATE_IN_COUNT_OBJECT,
        STATE_AT_COUNT,
        STATE_AT_EOSE_SUBSCRIPTION_ID,
        STATE_AT_OK_EVENT_ID,
        STATE_AT_OK_BOOLEAN,
        STATE_AT_OK_MESSAGE,
        STATE_AT_NOTICE_MESSAGE,
        STATE_AT_EVENT_SUBSCRIPTION_ID,
        STATE_AT_EVENT,
        STATE_ENDED
    };

    ReaderState state = STATE_STARTED;
    RelayMessage* result;
    char* buffer;

    bool stop() {
        state = STATE_ENDED;
        return false;
    }
    bool next(ReaderState next_state) {
        state = next_state;
        return true;
    }
    bool error() {
        return false;
    }

    bool Null() {
        return error();
    }
    bool Bool(bool b) {
        if (state == STATE_AT_OK_BOOLEAN) {
            result->ok.ok = b;
            return next(STATE_AT_OK_MESSAGE);
        }
        return error();
    }
    bool Int(int i) {
        return (i >= 0) ? Uint64(i) : error();
    }
    bool Uint(unsigned u) {
        return Uint64(u);
    }
    bool Int64(int64_t i) {
        return (i >= 0) ? Uint64(i) : error();
    }
    bool Uint64(uint64_t u) {
        if (state == STATE_AT_COUNT) {
            result->count.count = u;
            return stop();
        }
        return error();
    }
    bool Double(double d) {
        return false;
    }
    bool String(const char* str, rapidjson::SizeType length, bool copy) {
        if (state == STATE_AT_MESSAGE_TYPE) {
            if (strncmp("AUTH", str, length) == 0) {
                result->type = RelayMessage::AUTH;
                return next(STATE_AT_AUTH_CHALLENGE_STRING);
            } else if (strncmp("COUNT", str, length) == 0) {
                result->type = RelayMessage::COUNT;
                return next(STATE_AT_COUNT_SUBSCRIPTION_ID);
            } else if (strncmp("EOSE", str, length) == 0) {
                result->type = RelayMessage::EOSE;
                return next(STATE_AT_EOSE_SUBSCRIPTION_ID);
            } else if (strncmp("EVENT", str, length) == 0) {
                result->type = RelayMessage::EVENT;
                return next(STATE_AT_EVENT_SUBSCRIPTION_ID);
            } else if (strncmp("NOTICE", str, length) == 0) {
                result->type = RelayMessage::NOTICE;
                return next(STATE_AT_NOTICE_MESSAGE);
            } else if (strncmp("OK", str, length) == 0) {
                result->type = RelayMessage::OK;
                return next(STATE_AT_OK_EVENT_ID);
            }
            return error();

        } else if (state == STATE_AT_COUNT_SUBSCRIPTION_ID) {
            if (length >= RelayMessage::SUB_ID_MAX_LEN) return false;
            strncpy(result->count.subscription_id, str, length);
            result->count.subscription_id[length] = '\0';
            return next(STATE_AT_COUNT_OBJECT);

        } else if (state == STATE_AT_EOSE_SUBSCRIPTION_ID) {
            if (length >= RelayMessage::SUB_ID_MAX_LEN) return false;
            strncpy(result->eose.subscription_id, str, length);
            result->eose.subscription_id[length] = '\0';
            return stop();

        } else if (state == STATE_AT_EVENT_SUBSCRIPTION_ID) {
            if (length >= RelayMessage::SUB_ID_MAX_LEN) return false;
            strncpy(result->event.subscription_id, str, length);
            result->eose.subscription_id[length] = '\0';
            return next(STATE_AT_EVENT);

        } else if (state == STATE_AT_OK_EVENT_ID) {
            if (length != 2 * sizeof(EventId)) return false;
            if (!hex_decode(result->ok.event_id, str, sizeof(EventId))) return false;
            return next(STATE_AT_OK_BOOLEAN);

        } else if (state == STATE_AT_AUTH_CHALLENGE_STRING ||
                   state == STATE_AT_OK_MESSAGE ||
                   state == STATE_AT_NOTICE_MESSAGE) {
            strncpy(buffer, str, length);
            buffer[length] = '\0';

            if (result->type == RelayMessage::AUTH) {
                result->auth.challenge = buffer;
            } else if (result->type == RelayMessage::OK) {
                result->ok.message = buffer;
            } else if (result->type == RelayMessage::NOTICE) {
                result->notice.message = buffer;
            }

            return stop();
        }

        return error();
    }
    bool StartObject() {
        if (state == STATE_AT_COUNT_OBJECT) {
            return next(STATE_IN_COUNT_OBJECT);
        } else if (state == STATE_AT_EVENT) {
            return stop();
        }
        return error();
    }
    bool Key(const char* str, rapidjson::SizeType length, bool copy) {
        if (state == STATE_IN_COUNT_OBJECT &&
            strncmp("count", str, length) == 0) {
            return next(STATE_AT_COUNT);
        }
        return error();
    }
    bool EndObject(rapidjson::SizeType memberCount) {
        return error();
    }
    bool StartArray() {
        if (state == STATE_STARTED) {
            return next(STATE_AT_MESSAGE_TYPE);
        }
        return error();
    }
    bool EndArray(rapidjson::SizeType elementCount) {
        return error();
    }
};

bool relay_message_parse(const char* input, size_t input_len, uint8_t* buffer, RelayMessage* result) {

    RelayMessageReader handler;
    handler.result = result;
    handler.buffer = (char*)buffer;

    auto stack_size = max(input_len, 512);
    char stack_memory[stack_size];
    rapidjson::MemoryPoolAllocator<> allocator(stack_memory, stack_size);
    rapidjson::GenericReader<rapidjson::UTF8<>, rapidjson::UTF8<>, rapidjson::MemoryPoolAllocator<>> reader(&allocator);

    rapidjson::StringStream stream(input);
    reader.Parse(stream, handler);

    if (reader.HasParseError() && handler.state != RelayMessageReader::STATE_ENDED) {
        return false;
    }

    if (result->type == RelayMessage::EVENT) {
        result->event.input = &input[stream.Tell() - 1];
        result->event.input_len = input_len - (stream.Tell() - 1);
    }

    return true;
}
