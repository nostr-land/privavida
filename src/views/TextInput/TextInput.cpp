//
//  TextInput.cpp
//  ddui
//
//  Created by Bartholomew Joyce on 18/03/2018.
//  Copyright © 2018 Bartholomew Joyce All rights reserved.
//

#include "TextInput.hpp"
#include <app.hpp>
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
    bool is_editing = (
        ui::text_input->controller_id == &state.ref ||
        ui::simple_tap(0, 0, ui::view.width, ui::view.height)
    );

    nvgBeginPath(ui::vg);
    nvgFillColor(ui::vg, is_editing ? styles->bg_color_focused : styles->bg_color);
    nvgRoundedRect(ui::vg, 0, 0, ui::view.width, ui::view.height, styles->border_radius);
    nvgFill(ui::vg);

    if (styles->border_width) {
        nvgStrokeWidth(ui::vg, styles->border_width);
        nvgStrokeColor(ui::vg, is_editing ? styles->border_color_focused : styles->border_color);
        nvgStroke(ui::vg);
    }

    if (is_editing) {
        auto text_input = *ui::text_input;
        text_input.controller_id = &state.ref;
        ui::to_screen_rect(
            styles->padding,
            styles->padding,
            ui::view.width  - 2 * styles->padding,
            ui::view.height - 2 * styles->padding,
            &text_input.x, &text_input.y, &text_input.width, &text_input.height
        );
        text_input.font_size = styles->font_size;
        text_input.line_height = 1.0;
        text_input.text_color = styles->text_color;
        text_input.flags = 0;
        if (ui::text_input->controller_id != &state.ref) {
            text_input.content = "";
        }
        ui::text_input_set(&text_input);
    }
}

void TextInput::dismiss(const State* state) {
    if (TextInput::is_editing(state)) {
        ui::text_input_clear();
    }
}
