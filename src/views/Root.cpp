//
//  Root.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 10/05/2023.
//

#include "Root.hpp"
#include "../ui.hpp"
#include "ScrollView.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

constexpr int num_lines = 50;
constexpr int max_line_len = 100;
static char* lines[num_lines];

static bool keyboard_open = false;
static int selected_idx = 0;

static ScrollView::State sv_state;

void Root::init() {
    for (int i = 0; i < num_lines; ++i) {
        lines[i] = (char*)malloc(max_line_len);
        snprintf(lines[i], max_line_len, "%d", i + 1);
    }
}

void Root::update() {

    // Background
    nvgFillColor(ui::vg, (NVGcolor){ 0.0, 0.0, 0.0, 1.0 });
    nvgBeginPath(ui::vg);
    nvgRect(ui::vg, 0, 0, ui::view.width, ui::view.height);
    nvgFill(ui::vg);

    constexpr float BLOCK_HEIGHT = 100.0;

    ScrollView(&sv_state)
        .content_size(ui::view.width, BLOCK_HEIGHT * 50)
        .update([&](float visible_x, float visible_y, float visible_width, float visible_height) {

            int start_block = (int)(visible_y / BLOCK_HEIGHT);
            int end_block = start_block + (int)(visible_height / BLOCK_HEIGHT) + 1;

            for (int i = start_block; i <= end_block && i < num_lines; ++i) {

                int y = i * BLOCK_HEIGHT;

                if (i == selected_idx && keyboard_open) {
                    nvgBeginPath(ui::vg);
                    nvgRect(ui::vg, 0.0, y, ui::view.width, BLOCK_HEIGHT);
                    nvgFillColor(ui::vg, (NVGcolor){ 0.2, 0.2, 0.2, 1.0 });
                    nvgFill(ui::vg);
                } else if (i % 2 == 0) {
                    nvgBeginPath(ui::vg);
                    nvgRect(ui::vg, 0.0, y, ui::view.width, BLOCK_HEIGHT);
                    nvgFillColor(ui::vg, (NVGcolor){ 0.1, 0.1, 0.1, 1.0 });
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
                }

                nvgTextAlign(ui::vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
                nvgFillColor(ui::vg, (NVGcolor){ 1.0, 1.0, 1.0, 1.0 });
                nvgFontFace(ui::vg, "bold");
                nvgFontSize(ui::vg, 28.0);
                nvgText(ui::vg, ui::view.width * 0.5, y + BLOCK_HEIGHT * 0.5, lines[i], NULL);
            }

        });

    // Frame count
    float kb_x, kb_y, kb_width, kb_height;
    ui::keyboard_rect(&kb_x, &kb_y, &kb_width, &kb_height);
    nvgTextAlign(ui::vg, NVG_ALIGN_BOTTOM | NVG_ALIGN_LEFT);
    nvgFillColor(ui::vg, (NVGcolor){ 1.0, 1.0, 1.0, 1.0 });
    nvgFontFace(ui::vg, "mono");
    nvgFontSize(ui::vg, 16.0);
    char buf[32];
    static int frame_count = 0;
    snprintf(buf, 30, "Frame count: %d", frame_count++);
    nvgText(ui::vg, 10.0, kb_y - 10.0, buf, NULL);

}

void app_key_backspace() {
    auto len = strlen(lines[selected_idx]);
    if (len > 0) lines[selected_idx][len - 1] = '\0';

    // redraw.redraw(redraw.opaque_ptr);
}

void app_key_character(const char* ch) {
    if (strcmp(ch, "\n") == 0) {
        selected_idx++;
        return;
    }

    auto len = strlen(lines[selected_idx]);
    snprintf(lines[selected_idx] + len, max_line_len - len, "%s", ch);

    // redraw.redraw(redraw.opaque_ptr);
}
