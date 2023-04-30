//
//  profiles.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-22.
//

#pragma once
#include "events.hpp"
#include "../models/profile.hpp"

namespace data_layer {

void receive_profile(EventLocator event_loc);
const Profile* get_profile(const Pubkey* pubkey);
const Profile* get_or_request_profile(const Pubkey* pubkey);
void request_profile(const Pubkey* pubkey);
void batch_profile_requests();
void batch_profile_requests_send();

}
