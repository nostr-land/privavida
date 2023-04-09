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

static NVGcontext* vg;
static AppRedraw redraw;
static AppKeyboard keyboard;
static bool should_redraw = true;
static const char* temp_directory = NULL;

static AppTouchEvent touch_event_queue[1024];
static int touch_event_queue_size = 0;

void app_init(NVGcontext* vg_, AppRedraw redraw_, AppKeyboard keyboard_) {
    vg = vg_;
    redraw = redraw_;
    keyboard = keyboard_;
}

void app_set_temp_directory(const char* temp_directory_) {
    char* copy = (char*)malloc(strlen(temp_directory_) + 1);
    strcpy(copy, temp_directory_);
    temp_directory = copy;
}

void app_key_backspace() {
    printf("app_key_backspace()\n");
}

void app_key_character(const char* ch) {
    printf("app_key_backspace(\"%s\")\n", ch);
}

float clamp(float min, float val, float max) {
    return val > max ? max : val < min ? min : val;
}

enum DragState {
    IDLE,
    DRAGGING,
    ANIMATE_INERTIA
};

static float get_scroll() {
    
    static int frame_count = 0;
    frame_count += 1;
    
    static enum DragState state = IDLE;
    static float scroll_y = 0;
    static AppTouch touch1;
    static float scroll_initial_y;
    static float scroll_velocity_y;
    static int animation_start_time;

    // Process touches
    for (int i = 0; i < touch_event_queue_size; ++i) {
        auto& event = touch_event_queue[i];

        // Drag start?
        if (state != DRAGGING) {
            if (event.type == TOUCH_START) {
                state = DRAGGING;
                touch1 = event.touches_changed[0];
                scroll_initial_y = scroll_y;
            }
            continue;
        }

        // Find our touch
        AppTouch* touch1_changed = NULL;
        for (int i = 0; i < event.num_touches; ++i) {
            if (event.touches[i].id == touch1.id) {
                touch1_changed = &event.touches[i];
                continue;
            }
        }

        // Touch end or cancel
        if (!touch1_changed) {
            if (scroll_velocity_y > 0) {
                state = ANIMATE_INERTIA;
                scroll_initial_y = scroll_y;
                animation_start_time = frame_count;
            } else {
                state = IDLE;
            }
            continue;
        }

        // Touch move
        float dy = touch1_changed->y - touch1.y;
        float scroll_y_prev = scroll_y;
        scroll_y = scroll_initial_y - dy;
        scroll_velocity_y = scroll_y - scroll_y_prev;
    }
    touch_event_queue_size = 0;
    
    if (state == ANIMATE_INERTIA) {
        scroll_y += scroll_velocity_y;
        scroll_velocity_y *= 0.98;
        if (abs(scroll_velocity_y) < 0.1) {
            scroll_velocity_y = 0;
            state = IDLE;
        }
    }

    return scroll_y;
}

void app_render(float window_width, float window_height, float pixel_density) {
    nvgBeginFrame(vg, window_width, window_height, pixel_density);

    auto scroll_y = get_scroll();
    
    // Background
    nvgFillColor(vg, (NVGcolor){ 0.0, 0.0, 0.0, 1.0 });
    nvgBeginPath(vg);
    nvgRect(vg, 0, 0, window_width, window_height);
    nvgFill(vg);
    
    constexpr float BLOCK_HEIGHT = 100.0;
    scroll_y = clamp(0, scroll_y, BLOCK_HEIGHT * 50);
    int start_block = (int)(scroll_y / BLOCK_HEIGHT);
    int y = start_block * BLOCK_HEIGHT - scroll_y;
    
    for (int i = start_block;; ++i) {
        
        if (i % 2 == 0) {
            nvgBeginPath(vg);
            nvgRect(vg, 0.0, y, window_width, BLOCK_HEIGHT);
            nvgFillColor(vg, (NVGcolor){ 0.1, 0.1, 0.1, 1.0 });
            nvgFill(vg);
        }
        
        char buf[10];
        snprintf(buf, 10, "%d", i);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, (NVGcolor){ 1.0, 1.0, 1.0, 1.0 });
        nvgFontFace(vg, "bold");
        nvgFontSize(vg, 28.0);
        nvgText(vg, window_width * 0.5, y + BLOCK_HEIGHT * 0.5, buf, NULL);
        
        y += BLOCK_HEIGHT;
        if (y >= window_height) break;
    }

    // Frame count
    nvgTextAlign(vg, NVG_ALIGN_BOTTOM | NVG_ALIGN_LEFT);
    nvgFillColor(vg, (NVGcolor){ 1.0, 1.0, 1.0, 1.0 });
    nvgFontFace(vg, "mono");
    nvgFontSize(vg, 16.0);
    char buf[32];
    static int frame_count = 0;
    snprintf(buf, 30, "Frame count: %d", frame_count++);
    nvgText(vg, 10.0, window_height - 10.0, buf, NULL);

    redraw.redraw(redraw.opaque_ptr);
    nvgEndFrame(vg);
}

void app_touch_event(AppTouchEvent* event) {
    touch_event_queue[touch_event_queue_size++] = *event;
    redraw.redraw(redraw.opaque_ptr);
}
