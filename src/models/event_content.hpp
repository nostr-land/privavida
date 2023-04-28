//
//  event_content.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-28.
//

#pragma once

#include "event.hpp"
#include "../utils/stackbuffer.hpp"
#include <string>

void event_content_parse(const Event* event, StackArray<EventContentToken>& tokens, StackArray<NostrEntity*>& entities, StackBuffer& data);

size_t event_content_size_needed_for_copy(const Event* event, const StackArray<EventContentToken>& tokens, const StackArray<NostrEntity*>& entities);

void event_content_copy_result_into_event(Event* event, const StackArray<EventContentToken>& tokens, const StackArray<NostrEntity*>& entities);
