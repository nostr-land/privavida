//
//  handle_event.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 21/07/2023.
//

#pragma once
#include "../models/event.hpp"
#include "../data_layer/accounts.hpp"

namespace network {

void handle_event(const char* subscription_id, Event* event);
void account_response_handler(const AccountResponse* response);

}
