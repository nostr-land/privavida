//
//  relays.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-22.
//

#pragma once
#include "events.hpp"
#include "../models/relative.hpp"
#include "../models/relay_info.hpp"

namespace data_layer {

const RelayInfo* get_relay_info(RelayId relay_id);
const RelayInfo* get_relay_info(const char* relay_url);
Array<RelayId> get_default_relays();

}
