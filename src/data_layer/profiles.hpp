//
//  profiles.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-22.
//

#pragma once
#include "../models/profile.hpp"
#include <vector>

namespace data_layer {

extern std::vector<Profile*> profiles;
const Profile* get_profile(const Pubkey* pubkey);
const Profile* get_or_request_profile(const Pubkey* pubkey);
void request_profile(const Pubkey* pubkey);
void batch_profile_requests();
void batch_profile_requests_send();

}
