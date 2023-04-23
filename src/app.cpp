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
#include "utils/timer.hpp"
#include "views/Root.hpp"
#include "network/network.hpp"
#include "data_layer/accounts.hpp"

static AppTouchEvent touch_event_queue[1024];
static int touch_event_queue_size = 0;

void app_init(NVGcontext* vg_, AppText text_, AppStorage storage_, AppNetworking network_) {
    ui::vg = vg_;
    ui::text_input = text_;
    ui::redraw_requested = true;
    ui::storage = storage_;
    if (!data_layer::accounts_load()) return;
    network::init(network_);
    timer::init();

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

int app_wants_to_render() {
    timer::update();
    return (ui::redraw_requested || ui::has_key_event() || animation::is_animating());
}

void app_render(float window_width, float window_height, float pixel_density) {
    while (true) {
        nvgBeginFrame(ui::vg, window_width, window_height, pixel_density);
        ui::text_input_begin_frame();

        ui::gestures_process_touches(touch_event_queue, touch_event_queue_size);
        touch_event_queue_size = 0;

        animation::update_animation();

        ui::reset();
        ui::view.width = window_width;
        ui::view.height = window_height;

        ui::redraw_requested = false;
        ui::process_immediate_callbacks();
        ui::next_key_event();
        Root::update();
        ui::set_scroll(0, 0, 0, 0);

        if (!ui::redraw_requested) break;

        nvgCancelFrame(ui::vg);
    }

    nvgEndFrame(ui::vg);
    ui::text_input_end_frame();
}

void app_touch_event(AppTouchEvent* event) {
    touch_event_queue[touch_event_queue_size++] = *event;
    ui::redraw();
}

void app_scroll_event(int x, int y, int dx, int dy) {
    ui::set_scroll(x, y, dx, dy);
    ui::redraw();
}

void app_key_event(AppKeyEvent event) {
    ui::queue_key_event(event);
}

void app_keyboard_changed(int is_showing, float x, float y, float width, float height) {
    ui::keyboard_changed(is_showing, x, y, width, height);
}
