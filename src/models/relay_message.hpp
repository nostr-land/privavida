//
//  relay_message.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 20/07/2018.
//

#pragma once

#include "event.hpp"
#include "../utils/stackbuffer.hpp"
#include <string>

#define SUB_ID_MAX_LEN 65

struct RelayMessage {
    
    enum RelayMessageType {
        AUTH,
        COUNT,
        EOSE,
        EVENT,
        NOTICE,
        OK
    };

    struct RelayMessageAuth {
        const char* challenge;
    };
    struct RelayMessageCount {
        char subscription_id[SUB_ID_MAX_LEN];
        uint64_t count;
    };
    struct RelayMessageEose {
        char subscription_id[SUB_ID_MAX_LEN];
    };
    struct RelayMessageEvent {
        char subscription_id[SUB_ID_MAX_LEN];
        const char* input;
        size_t input_len;
    };
    struct RelayMessageNotice {
        const char* message;
    };
    struct RelayMessageOk {
        EventId event_id;
        bool ok;
        const char* message;
    };

    RelayMessageType type;
    union {
        RelayMessageAuth   auth;
        RelayMessageCount  count;
        RelayMessageEose   eose;
        RelayMessageEvent  event;
        RelayMessageNotice notice;
        RelayMessageOk     ok;
    };
};

bool relay_message_parse(const char* input, size_t input_len, StackBuffer* stack_buffer, RelayMessage* result);
