//
//  event_stringify.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-10.
//

#pragma once

#include "event.hpp"
#include "../utils/stackbuffer.hpp"

const char* event_stringify(const Event* event, StackBuffer* stack_buffer, bool wrap_in_event_message = false);
// When wrap_in_event_message is false you will get back <nostr event JSON>
// When wrap_in_event_message is true  you will get back ["EVENT",<nostr event JSON>]
