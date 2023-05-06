//
//  relay_message.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 20/07/2018.
//

#pragma once

#include "event.hpp"
#include <string>

static const size_t SUB_ID_MAX_LEN = 65;

struct RelayToClientMessage {

    enum MessageType {
        AUTH,
        COUNT,
        EOSE,
        EVENT,
        NOTICE,
        OK
    };

    struct MessageAuth {
        const char* challenge;
    };
    struct MessageCount {
        char subscription_id[SUB_ID_MAX_LEN];
        uint64_t count;
    };
    struct MessageEose {
        char subscription_id[SUB_ID_MAX_LEN];
    };
    struct MessageEvent {
        char subscription_id[SUB_ID_MAX_LEN];
        const char* input;
        size_t input_len;
    };
    struct MessageNotice {
        const char* message;
    };
    struct MessageOk {
        uint8_t event_id[sizeof(EventId)];
        bool ok;
        const char* message;
    };

    MessageType type;
    union {
        MessageAuth   auth;
        MessageCount  count;
        MessageEose   eose;
        MessageEvent  event;
        MessageNotice notice;
        MessageOk     ok;
    };
};

bool relay_to_client_message_parse(const char* input, size_t input_len, uint8_t* buffer, RelayToClientMessage* result);
