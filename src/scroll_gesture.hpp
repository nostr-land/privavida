//
//  scroll_gesture.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 09/05/2023.
//

#pragma once

extern "C" {
#include "app.h"
}

struct ScrollGesture {
    enum DragState {
        IDLE,
        DRAGGING,
        ANIMATE_INERTIA
    };

    struct State {
        enum DragState state = IDLE;
        float scroll_y = 0;
        AppTouch touch1;
        float scroll_initial_y;
        float scroll_velocity_y;
    };

    ScrollGesture(ScrollGesture::State* state) : state(*state) {};
    ScrollGesture& touch_events(AppTouchEvent* events, int num_events) {
        this->events = events;
        this->num_events = num_events;
        return *this;
    };
    ScrollGesture& bounds(float bounds_min, float bounds_max) {
        this->bounds_min = bounds_min;
        this->bounds_max = bounds_max;
        return *this;
    }
    ScrollGesture& min_threshold(float threshold) {
        this->threshold = threshold;
        return *this;
    }
    float update(AppRedraw redraw);

    State& state;
    AppTouchEvent* events;
    int num_events;
    float bounds_min = 0, bounds_max = 1000;
    float threshold = 0;
};
