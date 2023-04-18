//
//  event_parse.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 18/07/2018.
//

#pragma once

#include "event.hpp"
#include <string>

enum EventParseError {
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

struct EventParseResult {
    uint32_t text_content_len;
    uint32_t num_tags;
    uint32_t num_tag_values;
};

EventParseError event_parse(const char* input, size_t input_len, uint8_t* buffer, EventParseResult& result);

size_t event_get_size(const EventParseResult& result);

void event_create(Event* event, const uint8_t* buffer, const EventParseResult& result);
