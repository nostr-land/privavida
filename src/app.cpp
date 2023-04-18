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
#include "models/event_parse.hpp"
#include "models/event_stringify.hpp"
#include "models/hex.hpp"

static AppTouchEvent touch_event_queue[1024];
static int touch_event_queue_size = 0;

static AppNetworking network;

void app_init(NVGcontext* vg_, AppKeyboard keyboard_, AppStorage storage_, AppNetworking network_) {
    ui::vg = vg_;
    ui::keyboard = keyboard_;
    ui::redraw_requested = true;
    ui::storage = storage_;

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

    network = network_;
    auto socket = network.websocket_open(network.opaque_ptr, "wss://relay.damus.io");
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

void app_websocket_event(const AppWebsocketEvent* event) {
    if (event->type == WEBSOCKET_OPEN) {
        printf("websocket open!\n");
        const char* req = "[\"REQ\",\"sub\",{\"authors\":[\"489ac583fc30cfbee0095dd736ec46468faa8b187e311fda6269c4e18284ed0c\"],\"kinds\":[1]}]";
        network.websocket_send(network.opaque_ptr, event->ws, req);
    } else if (event->type == WEBSOCKET_CLOSE) {
        printf("websocket close!\n");
    } else if (event->type == WEBSOCKET_ERROR) {
        printf("websocket error!\n");
    } else if (event->type != WEBSOCKET_MESSAGE) {
        return;
    }

    const char* START = "[\"EVENT\",\"sub\",";
    if (strncmp(START, event->data, strlen(START)) != 0) {
        printf("message: %s\n", event->data);
        return;
    }

    int input_len = event->data_length - strlen(START) - 1;
    char input[event->data_length];
    strncpy(input, event->data + strlen(START), input_len);
    input[input_len] = '\0';

    uint8_t tlv[input_len];
    EventParseResult res;
    EventParseError err = event_parse(input, input_len, tlv, res);
    if (err) {
        printf("message invalid (parse error): %d\n", (int)err);
        return;
    }

    Event* nostr_event = (Event*)malloc(res.event_size);
    event_create(nostr_event, tlv, res);

    if (!event_validate(nostr_event)) {
        char id_given[65];
        hex_encode(id_given, nostr_event->id, 32);
        id_given[64] = '\0';

        event_compute_hash(nostr_event, nostr_event->id);
        char id_expected[65];
        hex_encode(id_expected, nostr_event->id, 32);
        id_expected[64] = '\0';

        printf("event is invalid id = %s vs. %s\n", id_given, id_expected);
        printf("content: %s\n", input);
    }
}

void app_touch_event(AppTouchEvent* event) {
    touch_event_queue[touch_event_queue_size++] = *event;
    ui::redraw();
}

void app_scroll_event(int x, int y, int dx, int dy) {
    ui::set_scroll(x, y, dx, dy);
    ui::redraw();
}
