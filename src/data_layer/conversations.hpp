//
//  conversations.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-21.
//

#pragma once

#include "events.hpp"
#include <vector>

namespace data_layer {

struct Message {

    enum Type {
        DIRECT_MESSAGE,
        INVITE
    };

    Type type;
    EventLocator event_loc;
};

struct Conversation {
    Pubkey counterparty;
    std::vector<Pubkey> aliases;
    std::vector<Message> messages;
    uint64_t last_active_time;
};

extern std::vector<Conversation> conversations;
extern std::vector<int> conversations_sorted;

void receive_direct_message(EventLocator event_loc);
void send_direct_message(int conversation_id, const char* message_text);

}
