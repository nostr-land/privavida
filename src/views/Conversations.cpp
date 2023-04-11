//
//  Conversations.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 11/05/2023.
//

#include "Conversations.hpp"
#include "ScrollView.hpp"
#include "SubView.hpp"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

constexpr int num_lines = 50;
constexpr int max_line_len = 100;
static char* lines[num_lines];

struct Contact {
    Contact(const char* name, const char* line1, const char* line2) : name(name), line1(line1), line2(line2) {}
    const char* name;
    const char* line1;
    const char* line2;
};

static std::vector<Contact> contact_list = {
    Contact("John Stone", "what time can u be at mine tmr?", "19:00 could be good for me"),
    Contact("Ponnappa Priya", "Mathieu: Alright parfait", ""),
    Contact("Mia Wong", "That is working out nicely then!", ""),
    Contact("Peter Stanbridge", "I'm on my way back right now", ""),
    Contact("Natalie Lee-Walsh", "We need to have you out at the next", "large event. Some really incredible..."),
    Contact("Ang Li", "Comme ça t'es au courant !", ""),
    Contact("Nguta Ithya", "Beatrix: Photo", ""),
    Contact("Tamzyn French", "Aller !", ""),
    Contact("Salome Simoes", "Hey man, just came across this -", "came in a couple months ago"),
    Contact("Trevor Virtue", "got it.", ""),
    Contact("Tarryn Campbell-Gillies", "thanksss", ""),
    Contact("Eugenia Anders", "You reacted ❤️ to \"Done!\"", ""),
    Contact("Andrew Kazantzis", "Ok moi pareil", ""),
    Contact("Verona Blair", "Mathis: Soirée chez moi vendredi", "soir, vous êtes tous invités, comme...")
};

static bool keyboard_open = false;
static int selected_idx = 0;

static ScrollView::State sv_state;

void Conversations::update() {

    float kb_x, kb_y, kb_width, kb_height;
    ui::keyboard_rect(&kb_x, &kb_y, &kb_width, &kb_height);
    
    // Background
    nvgFillColor(ui::vg, (NVGcolor){ 0.0, 0.0, 0.0, 1.0 });
    nvgBeginPath(ui::vg);
    nvgRect(ui::vg, 0, 0, ui::view.width, ui::view.height);
    nvgFill(ui::vg);

    // Header
    constexpr float HEADER_HEIGHT = 70.0;
    {
        SubView sub(0, 0, ui::view.width, HEADER_HEIGHT);
        nvgBeginPath(ui::vg);
        nvgRect(ui::vg, 0, 0, ui::view.width, ui::view.height);
        nvgFillColor(ui::vg, (NVGcolor){ 0.1, 0.1, 0.2, 1.0 });
        nvgFill(ui::vg);
    }

    // ScrollView
    constexpr float BLOCK_HEIGHT = 80.0;
    {
        SubView sub(0, HEADER_HEIGHT, ui::view.width, kb_y - HEADER_HEIGHT);
        ScrollView sv(&sv_state);
        sv.inner_size(ui::view.width, contact_list.size() * BLOCK_HEIGHT).update();

        int start_block = (int)(sv.state.scroll_y / BLOCK_HEIGHT);
        if (start_block < 0) start_block = 0;
        int end_block = start_block + (int)(sv.outer_height / BLOCK_HEIGHT) + 1;

        for (int i = start_block; i <= end_block && i < contact_list.size(); ++i) {

            auto& contact = contact_list[i];
            int y = i * BLOCK_HEIGHT;
            
            if (i == selected_idx && keyboard_open) {
                nvgBeginPath(ui::vg);
                nvgRect(ui::vg, 0.0, y, ui::view.width, BLOCK_HEIGHT);
                nvgFillColor(ui::vg, (NVGcolor){ 0.2, 0.2, 0.2, 1.0 });
                nvgFill(ui::vg);
            }

            if (ui::simple_tap(0, y, ui::view.width, BLOCK_HEIGHT)) {
                if (keyboard_open) {
                    if (selected_idx == i) {
                        keyboard_open = false;
                        ui::keyboard_close();
                    }
                } else {
                    keyboard_open = true;
                    ui::keyboard_open();
                }
                selected_idx = i;
                return ui::redraw();
            }

            nvgFillColor(ui::vg, (NVGcolor){ 0.5, 0.5, 0.7, 1.0 });
            nvgBeginPath(ui::vg);
            nvgCircle(ui::vg, 0.5 * BLOCK_HEIGHT, y + 0.5 * BLOCK_HEIGHT, 0.5 * BLOCK_HEIGHT - 10.0);
            nvgFill(ui::vg);
            
            constexpr float CONTENT_PADDING = 10.0;
            float CONTENT_HEIGHT = BLOCK_HEIGHT - 2 * CONTENT_PADDING;

            nvgTextAlign(ui::vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgFillColor(ui::vg, (NVGcolor){ 1.0, 1.0, 1.0, 1.0 });
            nvgFontSize(ui::vg, 16.0);
            nvgFontFace(ui::vg, "bold");
            
            nvgText(ui::vg, BLOCK_HEIGHT, y + CONTENT_PADDING + CONTENT_HEIGHT * (1.0 / 6.0), contact.name, NULL);
            
            nvgFillColor(ui::vg, (NVGcolor){ 0.8, 0.8, 0.8, 1.0 });
            nvgFontSize(ui::vg, 15.0);
            nvgFontFace(ui::vg, "regular");
            nvgText(ui::vg, BLOCK_HEIGHT, y + CONTENT_PADDING + CONTENT_HEIGHT * (3.0 / 6.0), contact.line1, NULL);
            nvgText(ui::vg, BLOCK_HEIGHT, y + CONTENT_PADDING + CONTENT_HEIGHT * (5.0 / 6.0), contact.line2, NULL);

            nvgFillColor(ui::vg, (NVGcolor){ 0.2, 0.2, 0.2, 1.0 });
            nvgBeginPath(ui::vg);
            nvgRect(ui::vg, BLOCK_HEIGHT, y + BLOCK_HEIGHT, ui::view.width - BLOCK_HEIGHT, 0.5);
            nvgFill(ui::vg);
        }
    }

}

void app_key_backspace() {
    // auto len = strlen(lines[selected_idx]);
    // if (len > 0) lines[selected_idx][len - 1] = '\0';

    // redraw.redraw(redraw.opaque_ptr);
}

void app_key_character(const char* ch) {
    if (strcmp(ch, "\n") == 0) {
        selected_idx++;
        return;
    }

    // auto len = strlen(lines[selected_idx]);
    // snprintf(lines[selected_idx] + len, max_line_len - len, "%s", ch);

    // redraw.redraw(redraw.opaque_ptr);
}
