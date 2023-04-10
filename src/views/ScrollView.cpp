//
//  ScrollView.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 10/05/2023.
//

#include "ScrollView.hpp"
#include <math.h>

static float clamp(float min, float x, float max) {
    return x < min ? min : x > max ? max : x;
}

void ScrollView::update() {
    
    outer_width = ui::view.width;
    outer_height = ui::view.height;

    // Drag start?
    ui::GestureTouch touch;
    if (state.state != DRAGGING && touch_start(0, 0, outer_width, outer_height, &touch)) {
        if (state.state == ANIMATE_INERTIA) {
            ui::touch_accept(touch.id); // if we were animating and a new touch starts, we accept it
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
            ui::touch_accept(touch.id);
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
            ui::touch_accept(state.touch_id);
        }
    }

    // Inertial animation
    if (state.state == ANIMATE_INERTIA) {
        ui::redraw();
        state.scroll_y += state.scroll_velocity_y;
        state.scroll_velocity_y *= 0.97;
        if (abs(state.scroll_velocity_y) < 0.25) {
            state.scroll_velocity_y = 0;
            state.state = IDLE;
        }
    }

    state.scroll_y = clamp(0, state.scroll_y, inner_height - outer_height);

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
