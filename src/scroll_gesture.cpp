//
//  scroll_gesture.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 09/05/2023.
//

#include "scroll_gesture.hpp"
#include <math.h>

static float clamp(float min, float val, float max) {
    return val > max ? max : val < min ? min : val;
}

float ScrollGesture::update(AppRedraw redraw) {

    // Process touches
    for (int i = 0; i < num_events; ++i) {
        auto& event = events[i];

        // Drag start?
        if (state.state != DRAGGING) {
            if (event.type == TOUCH_START) {
                state.state = DRAGGING;
                state.touch1 = event.touches_changed[0];
                state.scroll_initial_y = state.scroll_y;
                state.scroll_velocity_y = 0;
            }
            continue;
        }

        // Find our touch
        AppTouch* touch1_changed = NULL;
        for (int i = 0; i < event.num_touches; ++i) {
            if (event.touches[i].id == state.touch1.id) {
                touch1_changed = &event.touches[i];
                continue;
            }
        }

        // Touch end or cancel
        if (!touch1_changed) {
            if (abs(state.scroll_velocity_y) > 0) {
                state.state = ANIMATE_INERTIA;
                state.scroll_initial_y = state.scroll_y;
            } else {
                state.state = IDLE;
            }
            continue;
        }

        // Touch move
        float dy = touch1_changed->y - state.touch1.y;
        float scroll_y_prev = state.scroll_y;
        state.scroll_y = state.scroll_initial_y - dy;
        state.scroll_velocity_y = state.scroll_y - scroll_y_prev;
    }

    if (state.state == ANIMATE_INERTIA) {
        redraw.redraw(redraw.opaque_ptr);
        state.scroll_y += state.scroll_velocity_y;
        state.scroll_velocity_y *= 0.97;
        if (abs(state.scroll_velocity_y) < 0.25) {
            state.scroll_velocity_y = 0;
            state.state = IDLE;
        }
    }

    state.scroll_y = clamp(bounds_min, state.scroll_y, bounds_max);
    return state.scroll_y;
}
