//
//  client_message.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 20/07/2018.
//

#pragma once

#include "filters.hpp"
#include "../utils/stackbuffer.hpp"

const char* client_message_req(const char* subscription_id, const Filters* filters, StackBuffer* stack_buffer);
const char* client_message_close(const char* subscription_id, StackBuffer* stack_buffer);
