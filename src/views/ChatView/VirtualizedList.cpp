//
//  VirtualizedList.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-21.
//

#include "VirtualizedList.hpp"

void VirtualizedList::clear_measurements(VirtualizedList::State* state) {
    state->offsets.clear();
}

void VirtualizedList::update(VirtualizedList::State* state, int number_of_elements,
                             std::function<float(int)> measure_element_height,
                             std::function<void(int)> update_element,
                             std::function<void()> update_space_below) {

    // Number of elements changed, clear measurements
    if (state->offsets.size() != number_of_elements + 1) {
        state->offsets.clear();
    }

    // If the width has changed, clear measurments
    if (state->width != ui::view.width) {
        state->offsets.clear();
        state->width = ui::view.width;
    }

    // Remeasure all offsets
    if (state->offsets.empty()) {
        state->offsets.reserve(number_of_elements + 1);
        state->offsets.push_back(0.0);
        float accumulator = 0.0;
        for (auto i = 0; i < number_of_elements; ++i) {
            accumulator += measure_element_height(i);
            state->offsets.push_back(accumulator);
        }
    }

    // Scroll view
    ScrollView sv(&state->sv_state);
    sv.inner_size(ui::view.width, state->offsets.back()).update();

    // View port
    auto lower_y = sv.state.scroll_y;
    auto upper_y = lower_y + sv.outer_height;

    // Find lower offset
    auto lower = 0;
    for (; lower < number_of_elements; ++lower) {
        if (state->offsets[lower + 1] > lower_y) {
            break;
        }
    }

    // Find upper offset
    auto upper = lower;
    for (; upper < number_of_elements; ++upper) {
        if (state->offsets[upper] > upper_y) {
            break;
        }
    }

    // Draw stuff
    float offset_a;
    float offset_b = state->offsets[lower];

    for (auto i = lower; i < upper; ++i) {
        offset_a = offset_b;
        offset_b = state->offsets[i + 1];
        
        ui::sub_view(0, offset_a, ui::view.width, offset_b - offset_a);
        update_element(i);
        ui::restore();
    }

    // Update space below
    float space_after = sv.outer_height - state->offsets.back();
    if (space_after > 0) {
        ui::sub_view(0, state->offsets.back(), ui::view.width, space_after);
        update_space_below();
        ui::restore();
    }

}
