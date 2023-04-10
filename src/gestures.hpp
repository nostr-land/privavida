//
//  gestures.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 09/05/2023.
//

#pragma once

extern "C" {
#include "app.h"
}

constexpr unsigned char TOUCH_ACCEPTED = 0x01; // Once a gesture accepts a touch no other gestures can use it
constexpr unsigned char TOUCH_ENDED    = 0x04; // A touch sticks around in our state for one extra frame with flag TOUCH_ENDED
constexpr unsigned char TOUCH_MOVED    = 0x08; // Once a touch has moved it will be labeled as a TOUCH_MOVED

struct GestureTouch {
    int id;
    int x, y;
    int initial_x, initial_y;
    unsigned char flags;
};

void gestures_process_touches(AppTouchEvent* events, int num_events);
bool touch_start(float x, float y, float width, float height, GestureTouch* touch);
void touch_accept(int touch_id);
bool touch_ended(int touch_id, GestureTouch* touch);
bool simple_tap(float x, float y, float width, float height);

struct ScrollGesture {
    enum DragState {
        IDLE,
        DRAGGING,
        ANIMATE_INERTIA
    };

    struct State {
        enum DragState state = IDLE;
        float scroll_y = 0;
        int touch_id;
        float scroll_initial_y;
        float scroll_velocity_y;
    };

    ScrollGesture(ScrollGesture::State* state) : state(*state) {};
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
    float bounds_min = 0, bounds_max = 1000;
    float threshold = 0;
};
