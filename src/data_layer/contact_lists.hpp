//
//  contact_lists.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-22.
//

#pragma once
#include "events.hpp"

namespace data_layer {

void receive_contact_list(EventLocator event_loc);
const Event* get_contact_list(const Pubkey* pubkey);
bool does_first_follow_second(const Pubkey* first, const Pubkey* second);

}
