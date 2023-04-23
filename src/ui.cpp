//
//  ui.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 10/05/2023.
//

#include "ui.hpp"
#include "app.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <functional>
#include <queue>

namespace ui {

// Redraw
bool redraw_requested;
void redraw() {
    redraw_requested = true;
}

static std::vector<std::function<void()>> immediate_callbacks;
void set_immediate(std::function<void()> callback) {
    immediate_callbacks.push_back(std::move(callback));
    redraw();
}
void process_immediate_callbacks() {
    static std::vector<std::function<void()>> immediate_callbacks_copy;
    std::swap(immediate_callbacks_copy, immediate_callbacks);
    for (auto& fn : immediate_callbacks_copy) {
        fn();
    }
    immediate_callbacks_copy.clear();
}

// Viewport
Viewport view;
NVGcontext* vg;
constexpr int MAX_SAVED_VIEWS = 128;
static Viewport saved_views[MAX_SAVED_VIEWS];
static int num_saved_views = 0;

void save() {
    nvgSave(vg);
    saved_views[num_saved_views++] = view;
}

void restore() {
    nvgRestore(vg);
    view = saved_views[--num_saved_views];
}

void reset() {
    nvgReset(vg);
    view = saved_views[0];
    num_saved_views = 1;
}

void sub_view(float x, float y, float width, float height) {
    save();
    nvgTranslate(vg, x, y);
    view.width = width;
    view.height = height;
}

void to_screen_point(float x, float y, float* sx, float* sy) {
    float xform[6], inv_xform[6];
    nvgCurrentTransform(vg, xform);
    nvgTransformInverse(inv_xform, xform);
    nvgTransformPoint(sx, sy, xform, x, y);
}

void to_screen_rect(float x, float y, float width, float height, float* sx, float* sy, float* swidth, float* sheight) {
    float xform[6], inv_xform[6];
    nvgCurrentTransform(vg, xform);
    nvgTransformInverse(inv_xform, xform);
    nvgTransformPoint(sx, sy, xform, x, y);
    nvgTransformPoint(swidth, sheight, xform, x + width, y + height);
    *swidth -= *sx;
    *sheight -= *sy;
}

void to_view_point(float x, float y, float* vx, float* vy) {
    float xform[6];
    nvgCurrentTransform(vg, xform);
    nvgTransformPoint(vx, vy, xform, x, y);
}

void to_view_rect(float x, float y, float width, float height, float* vx, float* vy, float* vwidth, float* vheight) {
    float xform[6];
    nvgCurrentTransform(vg, xform);
    nvgTransformPoint(vx, vy, xform, x, y);
    nvgTransformPoint(vwidth, vheight, xform, x + width, y + height);
    *vwidth -= *vx;
    *vheight -= *vy;
}


// Gestures

static GestureTouch touches[MAX_TOUCHES];
static int num_touches = 0;

void gestures_process_touches(AppTouchEvent* events, int num_events) {

    // Remove touches flagged as TOUCH_ENDED
    {
        GestureTouch touches_next[MAX_TOUCHES];
        int num_touches_next = 0;
        for (int i = 0; i < num_touches; ++i) {
            if (!(touches[i].flags & TOUCH_ENDED)) {
                touches_next[num_touches_next++] = touches[i];
            }
        }
        memcpy(touches, touches_next, sizeof(GestureTouch) * num_touches_next);
        num_touches = num_touches_next;
    }

    // Process new touch events
    for (int i = 0; i < num_events; ++i) {
        auto& event = events[i];
        for (int j = 0; j < event.num_touches_changed; ++j) {
            auto& touch = event.touches_changed[j];

            // Find our representation of the touch
            GestureTouch* gtouch = NULL;
            for (int k = 0; k < num_touches; ++k) {
                if (touches[k].id == touch.id) {
                    gtouch = &touches[k];
                    break;
                }
            }

            switch (event.type) {
                case TOUCH_START: {
                    gtouch = &touches[num_touches++];
                    gtouch->id = touch.id;
                    gtouch->x = gtouch->initial_x = touch.x;
                    gtouch->y = gtouch->initial_y = touch.y;
                    gtouch->flags = 0;
                    break;
                }

                case TOUCH_MOVE: {
                    if (!gtouch) {
                        break;
                    }

                    gtouch->x = touch.x;
                    gtouch->y = touch.y;
                    if (!(gtouch->flags & TOUCH_MOVED) &&
                        (abs(gtouch->x - gtouch->initial_x) + abs(gtouch->y - gtouch->initial_y) > 4)) {
                        gtouch->flags |= TOUCH_MOVED;
                    }
                    break;
                }

                case TOUCH_END:
                case TOUCH_CANCEL: {
                    if (!gtouch) {
                        break;
                    }

                    gtouch->flags |= TOUCH_ENDED;
                }
            }
        }
    }

}

/////// WE NEED PRIVATE ACCESS TO NVGstate TO FETCH THE SCISSOR
#define NVG_MAX_STATES 32

struct NVGstate {
    NVGcompositeOperationState compositeOperation;
    int shapeAntiAlias;
    NVGpaint fill;
    NVGpaint stroke;
    float strokeWidth;
    float miterLimit;
    int lineJoin;
    int lineCap;
    float alpha;
    float xform[6];
    NVGscissor scissor;
    float fontSize;
    float letterSpacing;
    float lineHeight;
    float fontBlur;
    int textAlign;
    int fontId;
};

struct NVGcontext_ {
    NVGparams params;
    float* commands;
    int ccommands;
    int ncommands;
    float commandx, commandy;
    NVGstate states[NVG_MAX_STATES];
    int nstates;
};

static NVGscissor* get_scissor() {
    auto vg_ = (NVGcontext_*)vg;
    return &vg_->states[vg_->nstates - 1].scissor;
}


static bool is_point_inside_rect(float* xform, float* extent, float x, float y) {
    float inv_xform[6];
    nvgTransformInverse(inv_xform, xform);

    float x2, y2;
    nvgTransformPoint(&x2, &y2, inv_xform, x, y);

    return (
        (-extent[0] <= x2 && x2 < extent[0]) &&
        (-extent[1] <= y2 && y2 < extent[1])
    );
}

static bool touch_inside(float touch_x, float touch_y, float x, float y, float width, float height) {
    float xform[6], extent[2];
    nvgCurrentTransform(vg, xform);
    
    extent[0] = width / 2;
    extent[1] = height / 2;

    nvgTransformPoint(&x, &y, xform, x + extent[0], y + extent[1]);
    xform[4] = x;
    xform[5] = y;
    
    if (!is_point_inside_rect(xform, extent, touch_x, touch_y)) {
        return false;
    }

    auto scissor = get_scissor();
    if (scissor->extent[0] < 0) {
        return true;
    }

    return is_point_inside_rect(scissor->xform, scissor->extent, touch_x, touch_y);
}

bool touch_start(float x, float y, float width, float height, GestureTouch* touch) {
    for (int i = 0; i < num_touches; ++i) {
        if ((touches[i].flags & (TOUCH_ENDED | TOUCH_ACCEPTED)) == 0 &&
            touch_inside(touches[i].initial_x, touches[i].initial_y, x, y, width, height)) {
            *touch = touches[i];
            return true;
        }
    }

    return false;
}

void touch_accept(int touch_id) {
    for (int i = 0; i < num_touches; ++i) {
        if (touches[i].id == touch_id) {
            touches[i].flags |= TOUCH_ACCEPTED;
            break;
        }
    }
}

bool touch_ended(int touch_id, GestureTouch* touch) {
    for (int i = 0; i < num_touches; ++i) {
        if (touches[i].id == touch_id) {
            *touch = touches[i];
            return touches[i].flags & TOUCH_ENDED;
        }
    }

    return true;
}

bool simple_tap(float x, float y, float width, float height) {
    for (int i = 0; i < num_touches; ++i) {
        // Looking for a touch that ended, that was never accepted,
        // and that never moved
        if (touches[i].flags == TOUCH_ENDED &&
            touch_inside(touches[i].x, touches[i].y, x, y, width, height)) {
            touch_accept(touches[i].id);
            return true;
        }
    }
    
    return false;
}


// Scroll events
struct ScrollData {
    int x, y;
    int dx, dy;
};
static ScrollData scroll_data = { 0 }; 
void set_scroll(int x, int y, int dx, int dy) {
    scroll_data.x = x;
    scroll_data.y = y;
    scroll_data.dx = dx;
    scroll_data.dy = dy;
}
bool get_scroll(float x, float y, float width, float height, float* dx, float* dy) {
    if ((scroll_data.dx == 0 && scroll_data.dy == 0) ||
        !touch_inside(scroll_data.x, scroll_data.y, x, y, width, height)) {
        return false;
    }

    *dx = scroll_data.dx;
    *dy = scroll_data.dy;
    scroll_data.dx = 0;
    scroll_data.dy = 0;
    return true;
}


// Keyboard
AppText text_input;
AppKeyEvent key_state;
constexpr int KEY_EVENT_QUEUE_SIZE = 64;
static AppKeyEvent key_event_queue[KEY_EVENT_QUEUE_SIZE];
static long key_event_queue_write = 0;
static long key_event_queue_read = 0;
static float keyboard_y_;
static bool keyboard_is_showing_;

static AppTextInputConfig prev_config_, next_config_;
static const void* prev_text_input_ = NULL;
static const void* next_text_input_ = NULL;
 
void text_input_begin_frame() {
    next_text_input_ = NULL;
}

void text_input_end_frame() {
    if (prev_text_input_ && !next_text_input_) {
        text_input.remove_text_input(text_input.opaque_ptr);
    } else if (next_text_input_) {
        text_input.update_text_input(text_input.opaque_ptr, &next_config_);
    }
    prev_text_input_ = next_text_input_;
    prev_config_ = next_config_;
}

bool controls_text_input(const void* id) {
    return (prev_text_input_ == id || next_text_input_ == id);
}

void set_text_input(const void* id, const AppTextInputConfig* config) {
    next_text_input_ = id;
    next_config_ = *config;
}

float keyboard_y() {
    float screen_y;
    if (keyboard_is_showing_) {
        screen_y = keyboard_y_;
    } else {
        screen_y = (num_saved_views == 1) ? view.height : saved_views[1].height;
    }

    float view_x, view_y;
    to_view_point(0, screen_y, &view_x, &view_y);
    return view_y;
}

void keyboard_changed(int is_showing, float x, float y, float width, float height) {
    keyboard_is_showing_ = is_showing;
    keyboard_y_ = y;
    redraw_requested = true;
}

void queue_key_event(AppKeyEvent event) {
    if (key_event_queue_write - key_event_queue_read < KEY_EVENT_QUEUE_SIZE) {
        key_event_queue[key_event_queue_write++ % KEY_EVENT_QUEUE_SIZE] = event;
    }
}

bool has_key_event() {
    return (key_event_queue_write > key_event_queue_read);
}

void next_key_event() {
    if (key_event_queue_write > key_event_queue_read) {
        key_state = key_event_queue[key_event_queue_read++ % KEY_EVENT_QUEUE_SIZE];
    } else {
        memset(&key_state, 0, sizeof(AppKeyEvent));
    }
}

// Storage
AppStorage storage;
const char* get_asset_name(const char* asset_name, const char* asset_type) {
    return storage.get_asset_name(asset_name, asset_type);
}
const char* get_user_data_path(const char* filename) {
    static char buf[256];
    snprintf(buf, sizeof(buf), "%s/%s", storage.user_data_dir, filename);
    return buf;
}
void user_data_flush() {
    if (storage.user_data_flush) {
        storage.user_data_flush();
    }
}

}
