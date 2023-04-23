//
//  TextInput.cpp
//  ddui
//
//  Created by Bartholomew Joyce on 18/03/2018.
//  Copyright © 2018 Bartholomew Joyce All rights reserved.
//

#include "TextInput.hpp"
#include "../../ui.hpp"
#include "../SubView.hpp"
#include <cstdlib>

TextInput::StyleOptions* TextInput::get_global_styles() {
    static StyleOptions styles;
    static bool did_init = false;
    if (did_init) {
        return &styles;
    }

    did_init = true;
    styles.padding = 8;
    styles.border_radius = 4;
    styles.border_width = 1;
    styles.font_size = 16;
    styles.text_color = ui::color(0x000000);
    styles.border_color = ui::color(0xc8c8c8);
    styles.border_color_focused = ui::color(0x3264ff);
    styles.bg_color = ui::color(0xffffff);
    styles.bg_color_focused = ui::color(0xffffff);
    return &styles;
}

TextInput::TextInput(State* _state)
    : state(*_state) {
    styles = get_global_styles();
}

TextInput& TextInput::set_styles(const StyleOptions* styles) {
    this->styles = styles;
    return *this;
}

void TextInput::update() {
    if (!state.is_editing && ui::simple_tap(0, 0, ui::view.width, ui::view.height)) {
        state.is_editing = true;
    }

    nvgBeginPath(ui::vg);
    nvgFillColor(ui::vg, state.is_editing ? styles->bg_color_focused : styles->bg_color);
    nvgRoundedRect(ui::vg, 0, 0, ui::view.width, ui::view.height, styles->border_radius);
    nvgFill(ui::vg);

    if (styles->border_width) {
        nvgStrokeWidth(ui::vg, styles->border_width);
        nvgStrokeColor(ui::vg, state.is_editing ? styles->border_color_focused : styles->border_color);
        nvgStroke(ui::vg);
    }

    if (state.is_editing) {
        AppTextInputConfig config;
        ui::to_screen_rect(
            styles->padding,
            styles->padding,
            ui::view.width  - 2 * styles->padding,
            ui::view.height - 2 * styles->padding,
            &config.x, &config.y, &config.width, &config.height
        );
        config.font_size = styles->font_size;
        config.line_height = 1.0;
        config.text_color = styles->text_color;
        config.flags = 0;
        config.text_content = "Hello, world!";
        ui::set_text_input(&config);
    }
}
