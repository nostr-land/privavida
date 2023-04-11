//
//  ui.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 10/05/2023.
//

#pragma once

#include <nanovg.h>
#include "app.h"

namespace ui {

// Redraw
extern bool redraw_requested;
void redraw();

// Viewport
struct Viewport {
    float width;
    float height;
};
void save();
void restore();
void reset();
void sub_view(float x, float y, float width, float height);

extern Viewport view;
extern NVGcontext* vg;

// Touch gestures
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

// Keyboard
extern AppKeyboard keyboard;
void keyboard_open();
void keyboard_close();
void keyboard_rect(float* x, float* y, float* width, float* height);

}
