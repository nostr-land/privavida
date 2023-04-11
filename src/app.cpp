//
//  app.c
//  privavida-core
//
//  Created by Bartholomew Joyce on 08/05/2023.
//

#include "app.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "ui.hpp"
#include "utils/animation.hpp"
#include "views/Root.hpp"

static const char* temp_directory = NULL;

static AppTouchEvent touch_event_queue[1024];
static int touch_event_queue_size = 0;

void app_init(NVGcontext* vg_, AppKeyboard keyboard_) {
    ui::vg = vg_;
    ui::keyboard = keyboard_;
    ui::redraw_requested = true;
    Root::init();
}

void app_set_temp_directory(const char* temp_directory_) {
    char* copy = (char*)malloc(strlen(temp_directory_) + 1);
    strcpy(copy, temp_directory_);
    temp_directory = copy;
}

int app_wants_to_render() {
    return (ui::redraw_requested || animation::is_animating());
}

void app_render(float window_width, float window_height, float pixel_density) {
    while (true) {
        nvgBeginFrame(ui::vg, window_width, window_height, pixel_density);

        ui::gestures_process_touches(touch_event_queue, touch_event_queue_size);
        touch_event_queue_size = 0;

        animation::update_animation();

        ui::reset();
        ui::view.width = window_width;
        ui::view.height = window_height;

        ui::redraw_requested = false;
        Root::update();
        if (!ui::redraw_requested) break;

        nvgCancelFrame(ui::vg);
    }

    nvgEndFrame(ui::vg);
}

void app_touch_event(AppTouchEvent* event) {
    touch_event_queue[touch_event_queue_size++] = *event;
    ui::redraw();
}
