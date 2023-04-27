//
//  Composer.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-22.
//

#include "Composer.hpp"
#include "../SubView.hpp"

const float MARGIN = 6;
const float PADDING = 8;
const float FONT_SIZE = 16;

float Composer::height() const {
    return 2 * MARGIN + 2 * PADDING + FONT_SIZE;
}

const char* Composer::update() {

    // Background
    nvgBeginPath(ui::vg);
    nvgFillColor(ui::vg, ui::color(0x2D232B));
    nvgRect(ui::vg, 0, 0, ui::view.width, ui::view.height);
    nvgFill(ui::vg);
    
    // Apply margin
    SubView sv(MARGIN, MARGIN, ui::view.width - 2 * MARGIN, ui::view.height - 2 * MARGIN);

    const float button_size = ui::view.height;
    const float border_radius = 0.5 * ui::view.height;

    // Text input
    {
        SubView sv(0, 0, ui::view.width - 2 * MARGIN - button_size, ui::view.height);

        auto styles = *TextInput::get_global_styles();
        styles.padding = PADDING;
        styles.border_radius = border_radius;
        styles.border_width = 2;
        styles.font_size = FONT_SIZE;
        styles.text_color = ui::color(0xffffff);
        styles.border_color = ui::color(0x4D434B);
        styles.border_color_focused = ui::color(0x883955);
        styles.bg_color = ui::color(0x000000);
        styles.bg_color_focused = ui::color(0x000000);

        TextInput(&text_state).set_styles(&styles).update();
    }
    
    // Send button
    {
        SubView sv(ui::view.width - MARGIN - button_size, 0, button_size, button_size);

        nvgBeginPath(ui::vg);
        nvgFillColor(ui::vg, TextInput::is_editing(&text_state) ? ui::color(0x883955) : ui::color(0x4D434B));
        nvgCircle(ui::vg, 0.5 * button_size, 0.5 * button_size, 0.5 * button_size);
        nvgFill(ui::vg);
        if (ui::simple_tap(0, 0, ui::view.width, ui::view.height)) {
            const char* message = TextInput::get_text(&text_state);
            TextInput::dismiss(&text_state);
            return message;
        }
    }

    return NULL;
}
