//
//  tap_gesture.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 09/05/2023.
//

#include "tap_gesture.hpp"
#include <math.h>

bool TapGesture::tapped() {

    // Process touches
    for (int i = 0; i < num_events; ++i) {
        auto& event = events[i];

        if (!state.is_tapping) {
            if (event.type == TOUCH_START) {
                state.is_tapping = true;
                state.touch = event.touches_changed[0];
            }
            continue;
        }

        AppTouch* touch_changed = NULL;
        for (int i = 0; i < event.num_touches; ++i) {
            if (event.touches[i].id == state.touch.id) {
                touch_changed = &event.touches[i];
                continue;
            }
        }

        if (!touch_changed) {
            state.is_tapping = false;
            return true;
        } else if (abs(state.touch.x - touch_changed->x) + abs(state.touch.y - touch_changed->y) > 5.0) {
            state.is_tapping = false;
            return false;
        }
    }

    return false;
}
