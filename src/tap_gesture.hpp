//
//  tap_gesture.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 09/05/2023.
//

#pragma once

extern "C" {
#include "app.h"
}

struct TapGesture {
    struct State {
        bool is_tapping;
        AppTouch touch;
    };

    TapGesture(State* state) : state(*state) {};
    TapGesture& touch_events(AppTouchEvent* events, int num_events) {
        this->events = events;
        this->num_events = num_events;
        return *this;
    };
    bool tapped();

    State& state;
    AppTouchEvent* events;
    int num_events;
};
