//
//  VirtualizedList.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-21.
//

#pragma once

#include "../ScrollView.hpp"
#include <vector>
#include <functional>

struct VirtualizedList {

    struct State {
        std::vector<float> offsets;
        float width;
        ScrollView::State sv_state;
    };

    static void clear_measurements(State* state);
    static void update(State* state, int number_of_elements,
                       std::function<float(int)> measure_element_height,
                       std::function<void(int)> update_element,
                       std::function<void()> update_space_below);

};
