//
//  app.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-24.
//

#pragma once

#include "nanovg.h"
#include <functional>

// This is the internal app.hpp interface used by privavida-core.

namespace app {

// Set immediate
void set_immediate(std::function<void()> callback);

// Storage
const char* get_asset_name(const char* asset_name, const char* asset_type);
const char* get_user_data_path(const char* filename);
void user_data_flush();

}


namespace ui {

extern NVGcontext* vg;

// Redraw
void redraw();

// Viewport
struct Viewport {
    float width;
    float height;
};
extern Viewport view;
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

// Touch gestures
constexpr unsigned char TOUCH_ACCEPTED = 0x01; // Once a gesture accepts a touch no other gestures can use it
constexpr unsigned char TOUCH_ENDED    = 0x04; // A touch sticks around in our state for one extra frame with flag TOUCH_ENDED
constexpr unsigned char TOUCH_MOVED    = 0x08; // Once a touch has moved it will be labeled as a TOUCH_MOVED
struct Touch {
    int id;
    int x, y;
    int initial_x, initial_y;
    unsigned char flags;
};
bool touch_start(float x, float y, float width, float height, Touch* touch);
void touch_accept(int touch_id);
bool touch_ended(int touch_id, Touch* touch);
bool simple_tap(float x, float y, float width, float height);


// Scroll events
// (This is just for desktop/web use)
bool get_scroll(float x, float y, float width, float height, float* dx, float* dy);



// Keyboard
struct TextInputConfig {
    // Positioning of the text input
    // enum AppTextAnchor anchor;
    float x, y, width, height;

    // Font and text settings
    float font_size, line_height;
    NVGcolor text_color;
    int flags;

    // Content
    const char* text_content;
};
bool controls_text_input(const void* id);
void set_text_input(const void* id, const TextInputConfig* config);
void keyboard_changed(int is_showing, float x, float y, float width, float height);
float keyboard_y();

}
