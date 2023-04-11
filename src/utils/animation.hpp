//
//  animation.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 13/07/2018.
//

#pragma once

namespace animation {
    void update_animation();

    void start(void* identifier);
    void stop(void* identifier);
    bool is_animating(void* identifier);
    double get_time_elapsed(void* identifier);
    double ease_in(double completion);
    double ease_out(double completion);
    double ease_in_out(double completion);
    bool is_animating();
}
