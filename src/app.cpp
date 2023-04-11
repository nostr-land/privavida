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
static AppGetAssetName get_asset_name;

void app_init(NVGcontext* vg_, AppKeyboard keyboard_, AppGetAssetName get_asset_name_) {
    ui::vg = vg_;
    ui::keyboard = keyboard_;
    ui::redraw_requested = true;
    ui::get_asset_name_fptr = get_asset_name_;

    // nvgCreateFont(vg_, "mono",     ui::get_asset_name("PTMono",          "ttf"));
    nvgCreateFont(vg_, "regular",  ui::get_asset_name("SFRegular",       "ttf"));
    // nvgCreateFont(vg_, "regulari", ui::get_asset_name("SFRegularItalic", "ttf"));
    // nvgCreateFont(vg_, "medium",   ui::get_asset_name("SFMedium",        "ttf"));
    // nvgCreateFont(vg_, "mediumi",  ui::get_asset_name("SFMediumItalic",  "ttf"));
    nvgCreateFont(vg_, "bold",     ui::get_asset_name("SFBold",          "ttf"));
    // nvgCreateFont(vg_, "boldi",    ui::get_asset_name("SFBoldItalic",    "ttf"));
    // nvgCreateFont(vg_, "thin",     ui::get_asset_name("SFThin",          "ttf"));
    // nvgCreateImage(vg_, ui::get_asset_name("profile", "jpeg"), 0);

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
        ui::set_scroll(0, 0, 0, 0);

        if (!ui::redraw_requested) break;

        nvgCancelFrame(ui::vg);
    }

    nvgEndFrame(ui::vg);
}

void app_touch_event(AppTouchEvent* event) {
    touch_event_queue[touch_event_queue_size++] = *event;
    ui::redraw();
}

void app_scroll_event(int x, int y, int dx, int dy) {
    ui::set_scroll(x, y, dx, dy);
}
