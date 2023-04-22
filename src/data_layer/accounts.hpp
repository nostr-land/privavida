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

extern int account_selected;
extern std::vector<Account> accounts;

bool accounts_load();

}
