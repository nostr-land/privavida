//
//  relay_message_parse.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 20/07/2018.
//

#pragma once

#include "event.hpp"
#include <string>

struct RelayMessage {
    static const size_t SUB_ID_MAX_LEN = 65;

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
        uint8_t event_id[sizeof(Event::id)];
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

bool relay_message_parse(const char* input, size_t input_len, uint8_t* buffer, RelayMessage* result);
