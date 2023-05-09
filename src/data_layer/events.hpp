//
//  events.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-30.
//

#pragma once
#include "../models/event.hpp"

typedef int EventLocator;

namespace data_layer {

void receive_event(Event* event, int32_t relay_id, uint64_t receipt_time);
void send_event(Event* event);
const Event* event(EventLocator event_locator);

}
