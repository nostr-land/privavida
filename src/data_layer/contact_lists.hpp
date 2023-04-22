//
//  contact_lists.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-22.
//

#pragma once
#include "../models/event.hpp"
#include <vector>

namespace data_layer {

extern std::vector<Event*> contact_lists;
const Event* get_contact_list(const Pubkey* pubkey);
bool does_first_follow_second(const Pubkey* first, const Pubkey* second);

}
