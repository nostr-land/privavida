//
//  ui.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 10/05/2023.
//

#pragma once

#include "app.h"
#include <functional>

namespace ui {

// Redraw
extern bool redraw_requested;
void redraw();
void set_immediate(std::function<void()> callback);
void process_immediate_callbacks();

// Viewport
struct Viewport {
    float width;
    float height;
};
void save();
void restore();
void reset();
void sub_view(float x, float y, float width, float height);
void to_screen_point(float x, float y, float* sx, float* sy);
void to_screen_rect(float x, float y, float width, float height, float* sx, float* sy, float* swidth, float* sheight);
void to_view_point(float x, float y, float* sx, float* sy);
void to_view_rect(float x, float y, float width, float height, float* sx, float* sy, float* swidth, float* sheight);
static inline NVGcolor color(int rgb, float a) {
    return (NVGcolor){ ((rgb >> 16) % 256) / 255.0f, ((rgb >> 8) % 256) / 255.0f, ((rgb) % 256) / 255.0f, a };
}
static inline NVGcolor color(int rgb) {
    return color(rgb, 1.0);
}

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

// Scroll events
// (This is just for desktop/web use)
void set_scroll(int x, int y, int dx, int dy);
bool get_scroll(float x, float y, float width, float height, float* dx, float* dy);

// Keyboard
extern AppText text_input;
void text_input_begin_frame();
void text_input_end_frame();
void set_text_input(const AppTextInputConfig* config);
void keyboard_changed(int is_showing, float x, float y, float width, float height);
float keyboard_y();

void queue_key_event(AppKeyEvent event);
bool has_key_event();
void next_key_event();
extern AppKeyEvent key_state;

// Storage
extern AppStorage storage;
const char* get_asset_name(const char* asset_name, const char* asset_type);
const char* get_user_data_path(const char* filename);
void user_data_flush();

}
