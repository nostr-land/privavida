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

    ScrollView& content_size(float width, float height) {
        this->content_width  = width;
        this->content_height = height;
        return *this;
    }
    ScrollView& min_threshold(float threshold) {
        this->threshold = threshold;
        return *this;
    }
    void update(std::function<void(float visible_x, float visible_y, float visible_width, float visible_height)> inner_update);

    State& state;
    float content_width, content_height;
    float threshold = 5;
};
