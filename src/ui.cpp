//
//  ui.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 10/05/2023.
//

#include "ui.hpp"
#include "app.h"
#include <string.h>
#include <math.h>

namespace ui {

// Redraw
AppRedraw redraw_;
void redraw() {
    redraw_.redraw(redraw_.opaque_ptr);
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
                        (fabsf(gtouch->x - gtouch->initial_x) + fabsf(gtouch->y - gtouch->initial_y) > 4)) {
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
            return true;
        }
    }
    
    return false;
}


// Keyboard
AppKeyboard keyboard;

void keyboard_open() {
    keyboard.open(keyboard.opaque_ptr);
}

void keyboard_close() {
    keyboard.close(keyboard.opaque_ptr);
}

void keyboard_rect(float* x, float* y, float* width, float* height) {
    keyboard.rect(keyboard.opaque_ptr, x, y, width, height);
}

}
