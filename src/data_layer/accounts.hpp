//
//  accounts.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-21.
//

#pragma once
#include "../models/account.hpp"
#include <vector>

namespace data_layer {

bool accounts_load();
const Account* current_account();

}
