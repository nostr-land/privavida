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

void receive_event(Event* event);
const Event* event(EventLocator event_locator);

}
