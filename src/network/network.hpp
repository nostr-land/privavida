//
//  network.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 20/07/2023.
//

#pragma once
#include "../models/event.hpp"
#include <app.h>
#include "../ui.hpp"
#include <vector>
#include <memory>

namespace network {

void init(AppNetworking networking);

extern std::vector<Event*> events;

}
