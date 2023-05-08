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

const RelayInfo* get_relay_info(int32_t relay_id);
const RelayInfo* get_relay_info(const char* relay_url);
Array<int32_t> get_default_relays();

}
