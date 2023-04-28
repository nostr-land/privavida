//
//  nip31.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-28.
//

#pragma once
#include "nostr_entity.hpp"

bool nip31_verify_invite(NostrEntity* invite, const Pubkey* recipient);
