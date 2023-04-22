//
//  Conversations.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 11/05/2023.
//

#include "Conversations.hpp"
#include "ScrollView.hpp"
#include "SubView.hpp"
#include "Root.hpp"
#include "../data_layer/conversations.hpp"
#include "../models/hex.hpp"
#include "../models/nostr_entity.hpp"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

constexpr int num_lines = 50;
constexpr int max_line_len = 100;
static char* lines[num_lines];

static ScrollView::State sv_state;

void Conversations::init() {
}

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
        sv.inner_size(ui::view.width, data_layer::conversations.size() * BLOCK_HEIGHT).update();

        int start_block = (int)(sv.state.scroll_y / BLOCK_HEIGHT);
        if (start_block < 0) start_block = 0;
        int end_block = start_block + (int)(sv.outer_height / BLOCK_HEIGHT) + 1;

        for (int i = start_block; i <= end_block && i < data_layer::conversations.size(); ++i) {

            auto& conv = data_layer::conversations[i];
            int y = i * BLOCK_HEIGHT;

            // Highlight if conversation is open
            if (Root::open_conversation() == i) {
                nvgBeginPath(ui::vg);
                nvgFillColor(ui::vg, (NVGcolor){ 0.2, 0.2, 0.2, 1.0 });
                nvgRect(ui::vg, 0, y, ui::view.width, BLOCK_HEIGHT);
                nvgFill(ui::vg);
            }

            if (ui::simple_tap(0, y, ui::view.width, BLOCK_HEIGHT)) {
                Root::push_view_chat(i);
                return ui::redraw();
            }

            constexpr float PROFILE_PADDING = 10.0;
            {
                SubView sub(PROFILE_PADDING, y + PROFILE_PADDING, BLOCK_HEIGHT - 2.0 * PROFILE_PADDING, BLOCK_HEIGHT - 2.0 * PROFILE_PADDING);
                nvgFillColor(ui::vg, (NVGcolor){ 0.5, 0.5, 0.5, 1.0 });
                nvgBeginPath(ui::vg);
                nvgCircle(ui::vg, 0.5 * ui::view.width, 0.5 * ui::view.width, 0.5 * ui::view.width);
                nvgFill(ui::vg);
            }

            constexpr float CONTENT_PADDING = 10.0;
            float CONTENT_HEIGHT = BLOCK_HEIGHT - 2 * CONTENT_PADDING;

            nvgTextAlign(ui::vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgFillColor(ui::vg, (NVGcolor){ 1.0, 1.0, 1.0, 1.0 });
            nvgFontSize(ui::vg, 16.0);
            nvgFontFace(ui::vg, "bold");
            
            char name[100];
            NostrEntity::encode_npub(&conv.counterparty, name, NULL);

            nvgText(ui::vg, BLOCK_HEIGHT, y + CONTENT_PADDING + CONTENT_HEIGHT * (1.0 / 6.0), name, NULL);

            auto event = conv.messages.front();

            char line1[100];
            NostrEntity::encode_note(&event->id, line1, NULL);

            const char* line2;
            if (event->kind != 4) {
                line2 = event->content.data.get(event);
            } else if (event->content_encryption == EVENT_CONTENT_DECRYPTED) {
                line2 = event->content.data.get(event);
            } else if (event->content_encryption == EVENT_CONTENT_DECRYPT_FAILED) {
                line2 = "Decrypt failed";
            } else {
                line2 = "Content encrypted";
            }
            
            nvgFillColor(ui::vg, (NVGcolor){ 0.8, 0.8, 0.8, 1.0 });
            nvgFontSize(ui::vg, 15.0);
            nvgFontFace(ui::vg, "regular");
            nvgText(ui::vg, BLOCK_HEIGHT, y + CONTENT_PADDING + CONTENT_HEIGHT * (3.0 / 6.0), line1, NULL);
            nvgText(ui::vg, BLOCK_HEIGHT, y + CONTENT_PADDING + CONTENT_HEIGHT * (5.0 / 6.0), line2, NULL);

            nvgFillColor(ui::vg, (NVGcolor){ 0.2, 0.2, 0.2, 1.0 });
            nvgBeginPath(ui::vg);
            nvgRect(ui::vg, BLOCK_HEIGHT, y + BLOCK_HEIGHT, ui::view.width - BLOCK_HEIGHT, 0.5);
            nvgFill(ui::vg);
        }
    }

}
