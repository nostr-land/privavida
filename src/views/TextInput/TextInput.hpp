//
//  TextInput.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-23.
//

#pragma once
#include "../../ui.hpp"

struct TextInput {

    struct State {
        bool is_editing = false;
    };

    // Styles
    struct StyleOptions {
        float padding;
        float border_radius;
        float border_width;
        float font_size;
        NVGcolor text_color;
        NVGcolor border_color;
        NVGcolor border_color_focused;
        NVGcolor bg_color;
        NVGcolor bg_color_focused;
    };
    static StyleOptions* get_global_styles();

    // Methods
    TextInput(State* state);
    TextInput& set_styles(const StyleOptions* styles);
    void update();

protected:
    State& state;
    const StyleOptions* styles;
};
