//
//  network.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 20/07/2023.
//

#pragma once
#include "../models/event.hpp"
#include <app.h>
#include "../ui.hpp"
#include <vector>
#include <memory>

namespace network {

void init(AppNetworking networking);

struct Conversation {
    Pubkey counterparty;
    std::vector<Event*> messages;
};

extern std::vector<Conversation> conversations;

}
