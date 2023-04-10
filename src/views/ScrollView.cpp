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

void ScrollView::update(std::function<void(float visible_x, float visible_y, float visible_width, float visible_height)> inner_update) {
    
    float container_width  = ui::view.width;
    float container_height = ui::view.height;

    // Drag start?
    ui::GestureTouch touch;
    if (state.state != DRAGGING && touch_start(0, 0, container_width, container_height, &touch)) {
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
        if (fabsf(state.scroll_velocity_y) > 0) {
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

        if (fabsf(dy) > threshold) {
            ui::touch_accept(state.touch_id);
        }
    }

    // Inertial animation
    if (state.state == ANIMATE_INERTIA) {
        ui::redraw();
        state.scroll_y += state.scroll_velocity_y;
        state.scroll_velocity_y *= 0.97;
        if (fabsf(state.scroll_velocity_y) < 0.25) {
            state.scroll_velocity_y = 0;
            state.state = IDLE;
        }
    }

    state.scroll_y = clamp(0, state.scroll_y, content_height - container_height);

    ui::save();
    nvgIntersectScissor(ui::vg, 0, 0, container_width, container_height);
    ui::sub_view(-state.scroll_x, -state.scroll_y,
                 container_width  > content_width  ? container_width :  content_width,
                 container_height > content_height ? container_height : content_height);
    inner_update(state.scroll_x, state.scroll_y, container_width, container_height);
    ui::restore();
    ui::restore();
}
