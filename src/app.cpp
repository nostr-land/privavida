//
//  app.c
//  privavida-core
//
//  Created by Bartholomew Joyce on 08/05/2023.
//

extern "C" {
#include "app.h"
}

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "scroll_gesture.hpp"
#include "tap_gesture.hpp"

static NVGcontext* vg;
static AppRedraw redraw;
static AppKeyboard keyboard;
static bool should_redraw = true;
static const char* temp_directory = NULL;

static AppTouchEvent touch_event_queue[1024];
static int touch_event_queue_size = 0;

constexpr int num_lines = 50;
constexpr int max_line_len = 100;
static char* lines[num_lines];

static bool keyboard_open = false;
static int selected_idx = 0;

void app_init(NVGcontext* vg_, AppRedraw redraw_, AppKeyboard keyboard_) {
    vg = vg_;
    redraw = redraw_;
    keyboard = keyboard_;
    
    for (int i = 0; i < num_lines; ++i) {
        lines[i] = (char*)malloc(max_line_len);
        snprintf(lines[i], max_line_len, "%d", i + 1);
    }
}

void app_set_temp_directory(const char* temp_directory_) {
    char* copy = (char*)malloc(strlen(temp_directory_) + 1);
    strcpy(copy, temp_directory_);
    temp_directory = copy;
}

void app_key_backspace() {

    auto len = strlen(lines[selected_idx]);
    if (len > 0) lines[selected_idx][len - 1] = '\0';

    redraw.redraw(redraw.opaque_ptr);
}

void app_key_character(const char* ch) {
    if (strcmp(ch, "\n") == 0) {
        selected_idx++;
        return;
    }

    auto len = strlen(lines[selected_idx]);
    snprintf(lines[selected_idx] + len, max_line_len - len, "%s", ch);

    redraw.redraw(redraw.opaque_ptr);
}

static ScrollGesture::State sg_state;
static TapGesture::State tg_state;

void app_render(float window_width, float window_height, float pixel_density) {
    nvgBeginFrame(vg, window_width, window_height, pixel_density);

    // Background
    nvgFillColor(vg, (NVGcolor){ 0.0, 0.0, 0.0, 1.0 });
    nvgBeginPath(vg);
    nvgRect(vg, 0, 0, window_width, window_height);
    nvgFill(vg);

    constexpr float BLOCK_HEIGHT = 100.0;
    auto scroll_y = ScrollGesture(&sg_state)
                        .bounds(0, BLOCK_HEIGHT * 50 - window_height)
                        .touch_events(touch_event_queue, touch_event_queue_size)
                        .update(redraw);
    int start_block = (int)(scroll_y / BLOCK_HEIGHT);
    int y = start_block * BLOCK_HEIGHT - scroll_y;
    

    if (TapGesture(&tg_state).touch_events(touch_event_queue, touch_event_queue_size).tapped()) {
        keyboard_open = !keyboard_open;
        if (keyboard_open) {
            keyboard.open(keyboard.opaque_ptr);
            selected_idx = 0;
        } else {
            keyboard.close(keyboard.opaque_ptr);
        }
    }
    touch_event_queue_size = 0;

    for (int i = start_block;; ++i) {
        
        if (i % 2 == 0) {
            nvgBeginPath(vg);
            nvgRect(vg, 0.0, y, window_width, BLOCK_HEIGHT);
            nvgFillColor(vg, (NVGcolor){ 0.1, 0.1, 0.1, 1.0 });
            nvgFill(vg);
        }

        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, (NVGcolor){ 1.0, 1.0, 1.0, 1.0 });
        nvgFontFace(vg, "bold");
        nvgFontSize(vg, 28.0);
        nvgText(vg, window_width * 0.5, y + BLOCK_HEIGHT * 0.5, lines[i], NULL);
        
        y += BLOCK_HEIGHT;
        if (y >= window_height) break;
    }

    // Frame count
//    nvgTextAlign(vg, NVG_ALIGN_BOTTOM | NVG_ALIGN_LEFT);
//    nvgFillColor(vg, (NVGcolor){ 1.0, 1.0, 1.0, 1.0 });
//    nvgFontFace(vg, "mono");
//    nvgFontSize(vg, 16.0);
//    char buf[32];
//    static int frame_count = 0;
//    snprintf(buf, 30, "Frame count: %d", frame_count++);
//    nvgText(vg, 10.0, window_height - 10.0, buf, NULL);

    nvgEndFrame(vg);
}

void app_touch_event(AppTouchEvent* event) {
    touch_event_queue[touch_event_queue_size++] = *event;
    redraw.redraw(redraw.opaque_ptr);
}
