//
//  gestures.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 09/05/2023.
//

#include "gestures.hpp"
#include <math.h>
#include <string.h>

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

bool touch_start(float x, float y, float width, float height, GestureTouch* touch) {
    for (int i = 0; i < num_touches; ++i) {
        if ((touches[i].flags & (TOUCH_ENDED | TOUCH_ACCEPTED)) == 0 &&
            touches[i].x >= x && touches[i].x < x + width &&
            touches[i].y >= y && touches[i].y < y + height) {
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
            touches[i].x >= x && touches[i].x < x + width &&
            touches[i].y >= y && touches[i].y < y + height) {
            return true;
        }
    }
    
    return false;
}

static float clamp(float min, float x, float max) {
    return x < min ? min : x > max ? max : x;
}

float ScrollGesture::update(AppRedraw redraw) {

    // Drag start?
    GestureTouch touch;
    if (state.state != DRAGGING && touch_start(0, 0, 4000, 4000, &touch)) {
        if (state.state == ANIMATE_INERTIA) {
            touch_accept(touch.id); // if we were animating and a new touch starts, we accept it
        }
        state.state = DRAGGING;
        state.touch_id = touch.id;
        state.scroll_initial_y = state.scroll_y;
        state.scroll_velocity_y = 0;
    }

    // Touch end?
    if (state.state == DRAGGING && touch_ended(state.touch_id, &touch)) {
        if (abs(state.scroll_velocity_y) > 0) {
            state.state = ANIMATE_INERTIA;
            state.scroll_initial_y = state.scroll_y;
            touch_accept(touch.id);
        } else {
            state.state = IDLE;
        }
    }

    // Update touch
    if (state.state == DRAGGING) {
        float dy = touch.y - touch.initial_y;
        float scroll_y_prev = state.scroll_y;
        state.scroll_y = state.scroll_initial_y - dy;
        state.scroll_velocity_y = state.scroll_y - scroll_y_prev;

        if (abs(dy) > threshold) {
            touch_accept(state.touch_id);
        }
    }

    // Inertial animation
    if (state.state == ANIMATE_INERTIA) {
        redraw.redraw(redraw.opaque_ptr);
        state.scroll_y += state.scroll_velocity_y;
        state.scroll_velocity_y *= 0.97;
        if (abs(state.scroll_velocity_y) < 0.25) {
            state.scroll_velocity_y = 0;
            state.state = IDLE;
        }
    }

    state.scroll_y = clamp(bounds_min, state.scroll_y, bounds_max);
    return state.scroll_y;
}
