//
//  conversations.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-21.
//

#pragma once
#include "../models/event.hpp"
#include <vector>

namespace data_layer {

struct Conversation {
    Pubkey counterparty;
    std::vector<Event*> messages;
};

extern std::vector<Conversation> conversations;

void send_direct_message(int conversation_id, const char* message_text);

}
