//
//  timer.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-22.
//

#pragma once
#include <functional>

namespace timer {

void init();
void update();

int set_timeout(std::function<void()> callback, long time_in_ms);
void clear_timeout(int timeout_id);
int set_interval(std::function<void()> callback, long time_in_ms);
void clear_interval(int interval_id);

}
