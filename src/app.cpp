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

// Implementation of a cube that can be
// moved around and scaled with touch
// gestures.

static float rect_x = 150, rect_y = 150;
static float rect_scale = 1.0;

void app_render(float window_width, float window_height, float pixel_density) {
    nvgBeginFrame(vg, window_width, window_height, pixel_density);

    nvgFillColor(vg, (NVGcolor){ 0.0, 0.0, 0.0, 1.0 });
    nvgBeginPath(vg);
    nvgRect(vg, 0, 0, window_width, window_height);
    nvgFill(vg);

    nvgBeginPath(vg);
    float rect_w = 100 * rect_scale;
    float rect_h = 100 * rect_scale;
    nvgRect(vg, rect_x - 0.5 * rect_w, rect_y - 0.5 * rect_h, rect_w, rect_h);
    nvgFillColor(vg, (NVGcolor){ 1.0, 0.0, 0.0, 1.0 });
    nvgFill(vg);

    nvgEndFrame(vg);
}

enum Gesture {
    NONE,
    MOVE,
    SCALE
};

void app_touch_event(AppTouchEvent* event) {
    static enum Gesture gesture = NONE;

    static float rect_ix, rect_iy, rect_iscale;
    static AppTouch touch1, touch2;
    
    redraw.redraw(redraw.opaque_ptr);

    if (gesture == NONE) {
        if (event->type == TOUCH_START) {
            gesture = MOVE;
            touch1 = event->touches_changed[0];
            rect_ix = rect_x;
            rect_iy = rect_y;
        }
        return;
    }
    
    if (gesture == MOVE) {
        if (event->type == TOUCH_START) {
            gesture = SCALE;
            for (int i = 0; i < event->num_touches; ++i) {
                if (event->touches[i].id == touch1.id) {
                    touch1 = event->touches[i];
                    break;
                }
            }
            touch2 = event->touches_changed[0];
            rect_ix = rect_x;
            rect_iy = rect_y;
            rect_iscale = rect_scale;
        } else if (event->type == TOUCH_MOVE) {
            
            // Fetch changed touches
            AppTouch* touch1_changed = &touch1;
            for (int i = 0; i < event->num_touches; ++i) {
                if (event->touches[i].id == touch1.id) {
                    touch1_changed = &event->touches[i];
                    break;
                }
            }

            // Update rect position
            float dx = touch1_changed->x - touch1.x;
            float dy = touch1_changed->y - touch1.y;
            rect_x = rect_ix + dx;
            rect_y = rect_iy + dy;

        } else { // TOUCH_END or TOUCH_MOVE
            gesture = NONE;
        }
        return;
    }

    if (gesture == SCALE) {
        if (event->type == TOUCH_START) {
            // ignore
        } else if (event->type == TOUCH_MOVE) {

            // Fetch changed touches
            AppTouch* touch1_changed = &touch1;
            AppTouch* touch2_changed = &touch2;
            for (int i = 0; i < event->num_touches; ++i) {
                if (event->touches[i].id == touch1.id) {
                    touch1_changed = &event->touches[i];
                } else if (event->touches[i].id == touch2.id) {
                    touch2_changed = &event->touches[i];
                }
            }

            // Update scale
            float dist_i;
            {
                float dx = touch2.x - touch1.x;
                float dy = touch2.y - touch1.y;
                dist_i = sqrtf(dx * dx + dy * dy);
            }
            float cx_i = (touch1.x + touch2.x) / 2;
            float cy_i = (touch1.y + touch2.y) / 2;
            float dist_now;
            {
                float dx = touch2_changed->x - touch1_changed->x;
                float dy = touch2_changed->y - touch1_changed->y;
                dist_now = sqrtf(dx * dx + dy * dy);
            }
            float cx_now = (touch1_changed->x + touch2_changed->x) / 2;
            float cy_now = (touch1_changed->y + touch2_changed->y) / 2;
            rect_scale = (dist_now / dist_i) * rect_iscale;
            rect_x = rect_ix + (cx_now - cx_i);
            rect_y = rect_iy + (cy_now - cy_i);

        } else { // TOUCH_END or TOUCH_MOVE

            // Fetch changed touches
            AppTouch* touch1_changed = NULL;
            AppTouch* touch2_changed = NULL;
            for (int i = 0; i < event->num_touches; ++i) {
                if (event->touches[i].id == touch1.id) {
                    touch1_changed = &event->touches[i];
                } else if (event->touches[i].id == touch2.id) {
                    touch2_changed = &event->touches[i];
                }
            }
            if (!touch1_changed && !touch2_changed) {
                gesture = NONE;
            } else if (touch1_changed) {
                gesture = MOVE;
                touch1 = *touch1_changed;
                rect_ix = rect_x;
                rect_iy = rect_y;
            } else if (touch2_changed) {
                gesture = MOVE;
                touch1 = *touch2_changed;
                rect_ix = rect_x;
                rect_iy = rect_y;
            }

        }
    }
}
