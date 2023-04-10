//
//  ScrollView.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 10/05/2023.
//

#pragma once

#include "../ui.hpp"
#include <functional>

struct ScrollView {
    enum DragState {
        IDLE,
        DRAGGING,
        ANIMATE_INERTIA
    };

    struct State {
        enum DragState state = IDLE;
        float scroll_x = 0;
        float scroll_y = 0;
        int touch_id;
        float scroll_initial_y;
        float scroll_velocity_y;
    };

    ScrollView(ScrollView::State* state) : state(*state) {};
    ~ScrollView();
    ScrollView(ScrollView&&) = delete;

    ScrollView& inner_size(float width, float height) {
        this->inner_width  = width;
        this->inner_height = height;
        return *this;
    }
    ScrollView& min_threshold(float threshold) {
        this->threshold = threshold;
        return *this;
    }
    void update();

    // Parameters and state
    State& state;
    float inner_width, inner_height;
    float outer_width, outer_height;

    float threshold = 5;
};
