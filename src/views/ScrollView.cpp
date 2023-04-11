//
//  ScrollView.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 10/05/2023.
//

#include "ScrollView.hpp"
#include "../utils/animation.hpp"
#include <math.h>

static float clamp(float min, float x, float max) {
    return x < min ? min : x > max ? max : x;
}

constexpr float RUBBER_BAND_DURATION = 0.4;
constexpr float RUBBER_BAND_SLACK = 0.5;
constexpr float INERTIA_DECELERATION = -1000.0;
constexpr float FPS = 60.0;

void ScrollView::update() {
    
    outer_width = ui::view.width;
    outer_height = ui::view.height;
    float max_scroll_y = inner_height > outer_height ? inner_height - outer_height : 0;

    // Drag start?
    ui::GestureTouch touch;
    if (state.state != DRAGGING && touch_start(0, 0, outer_width, outer_height, &touch)) {
        if (state.state == ANIMATE_INERTIA) {
            ui::touch_accept(touch.id); // if we were animating and a new touch starts, we accept it
        }
        state.state = DRAGGING;
        state.touch_id = touch.id;
        state.scroll_initial_y = state.scroll_y;
        state.scroll_velocity_y_mag = 0;
        state.scroll_velocity_y_dir = 0;
    }

    // Touch end?
    if (state.state == DRAGGING && touch_ended(state.touch_id, &touch)) {
        if (state.scroll_y < 0 || state.scroll_y > max_scroll_y) {
            state.state = ANIMATE_RUBBER_BAND;
            state.scroll_initial_y = state.scroll_y;
            ui::touch_accept(touch.id);
            animation::start((void*)&state);
        } else if (state.scroll_velocity_y_mag > 0) {
            state.state = ANIMATE_INERTIA;
            state.scroll_initial_y = state.scroll_y;
            ui::touch_accept(touch.id);
            animation::start((void*)&state);
        } else {
            state.state = IDLE;
        }
    }

    // Update touch
    if (state.state == DRAGGING) {
        float dy = touch.y - touch.initial_y;
        float scroll_y_next = state.scroll_initial_y - dy;
        state.scroll_velocity_y_mag = abs(scroll_y_next - state.scroll_y) * FPS;
        state.scroll_velocity_y_dir = scroll_y_next > state.scroll_y ? 1.0 : -1.0;

        if (abs(dy) > threshold) {
            ui::touch_accept(state.touch_id);
        }
    }

    // Outer height change?
    if (state.state == IDLE && state.scroll_y > max_scroll_y) {
        state.state = ANIMATE_RUBBER_BAND;
        state.scroll_initial_y = state.scroll_y;
        animation::start((void*)&state);
    }

    float dx, dy;
    if (ui::get_scroll(0, 0, ui::view.width, ui::view.height, &dx, &dy)) {
        state.scroll_y += dy;
        state.scroll_y = clamp(0, state.scroll_y, max_scroll_y);
    }

    // Update scroll y
    if (state.state == ANIMATE_INERTIA) {
        double elapsed = animation::get_time_elapsed((void*)&state);
        animation::start((void*)&state); // reset elapsed time so we get time between frames

        state.scroll_y += state.scroll_velocity_y_dir * state.scroll_velocity_y_mag * elapsed;
        bool out_of_bounds = (state.scroll_y < 0 || state.scroll_y > max_scroll_y);

        float deceleration;
        if (!out_of_bounds) {
            deceleration = INERTIA_DECELERATION;
        } else {
            float displacement = abs(state.scroll_y - clamp(0, state.scroll_y, max_scroll_y));
            deceleration = -displacement * 200.0;
        }

        state.scroll_velocity_y_mag += deceleration * elapsed;
        if (state.scroll_velocity_y_mag < 0.0) {
            if (out_of_bounds) {
                state.state = ANIMATE_RUBBER_BAND;
                state.scroll_initial_y = state.scroll_y;
            } else {
                animation::stop((void*)&state);
                state.state = IDLE;
            }
        }
    } else if (state.state == ANIMATE_RUBBER_BAND) {
        float scroll_target_y = clamp(0, state.scroll_y, max_scroll_y);
        auto ratio = animation::get_time_elapsed((void*)&state) / RUBBER_BAND_DURATION;
        if (ratio > 1.0) {
            state.scroll_y = scroll_target_y;
            state.state = IDLE;
        } else {
            state.scroll_y = state.scroll_initial_y + (scroll_target_y - state.scroll_initial_y) * animation::ease_out(ratio);
        }
    } else if (state.state == DRAGGING) {
        float dy = touch.y - touch.initial_y;
        state.scroll_y = state.scroll_initial_y - (touch.y - touch.initial_y);

        // Apply slack
        if (state.scroll_y < 0) {
            state.scroll_y = RUBBER_BAND_SLACK * state.scroll_y;
        } else if (state.scroll_y > max_scroll_y) {
            state.scroll_y = max_scroll_y + RUBBER_BAND_SLACK * (state.scroll_y - max_scroll_y);
        }
    }

    ui::save();
    nvgIntersectScissor(ui::vg, 0, 0, outer_width, outer_height);
    ui::sub_view(-state.scroll_x, -state.scroll_y,
                 outer_width  > inner_width  ? outer_width :  inner_width,
                 outer_height > inner_height ? outer_height : inner_height);
}

ScrollView::~ScrollView() {
    ui::restore();
    ui::restore();
}
