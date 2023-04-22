//
//  network.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 20/07/2023.
//

#pragma once
#include "../models/event.hpp"
#include <app.h>

namespace network {

void init(AppNetworking networking);
void send(const char* message);

}
