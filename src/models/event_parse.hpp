//
//  event_parse.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 18/07/2018.
//

#pragma once

#include "event.hpp"
#include <string>

enum ParseError {
    PARSE_NO_ERR,
    PARSE_ERR_EMPTY_INPUT,
    PARSE_ERR_INVALID_JSON,
    PARSE_ERR_INVALID_EVENT,
    PARSE_ERR_MISSING_ID,
    PARSE_ERR_MISSING_PUBKEY,
    PARSE_ERR_MISSING_SIG,
    PARSE_ERR_MISSING_KIND,
    PARSE_ERR_MISSING_CREATED_AT,
    PARSE_ERR_INVALID_ID,
    PARSE_ERR_INVALID_PUBKEY,
    PARSE_ERR_INVALID_SIG,
    PARSE_ERR_INVALID_KIND,
    PARSE_ERR_INVALID_CREATED_AT,
    PARSE_ERR_INVALID_CONTENT,
    PARSE_ERR_INVALID_TAGS,
    PARSE_ERR_DUPLICATE_ID,
    PARSE_ERR_DUPLICATE_PUBKEY,
    PARSE_ERR_DUPLICATE_SIG,
    PARSE_ERR_DUPLICATE_KIND,
    PARSE_ERR_DUPLICATE_CREATED_AT,
    PARSE_ERR_DUPLICATE_CONTENT,
    PARSE_ERR_DUPLICATE_TAGS,
};

enum RelayMessageType {
    MESSAGE_TYPE_AUTH,
    MESSAGE_TYPE_COUNT,
    MESSAGE_TYPE_EOSE,
    MESSAGE_TYPE_EVENT,
    MESSAGE_TYPE_NOTICE,
    MESSAGE_TYPE_OK
};

struct EventParseResult {
    uint32_t event_size;
    uint32_t num_tags;
    uint32_t num_tag_values;
};

struct RelayMessageParseResult {
    RelayMessageType message_type;
    char subscription_id[65];
    uint8_t event_id[sizeof(Event::id)];
    bool ok;
    const char* message;
    uint64_t count;

    bool has_event;
    const char* event_input;
    size_t event_input_len;
};

ParseError event_parse(const char* input, size_t input_len, uint8_t* tlv_out, EventParseResult& result);

void event_create(Event* event_out, const uint8_t* tlv, const EventParseResult& result);
