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
#include "../data_layer/accounts.hpp"
#include "../data_layer/conversations.hpp"
#include "../data_layer/profiles.hpp"
#include "../data_layer/contact_lists.hpp"
#include "../data_layer/images.hpp"
#include "../models/hex.hpp"
#include "../models/nostr_entity.hpp"
#include "../utils/animation.hpp"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static ScrollView::State sv_state;

void Conversations::init() {
}

void Conversations::update() {

    auto& account = data_layer::accounts[data_layer::account_selected];
    
    // Background
    nvgFillColor(ui::vg, (NVGcolor){ 0.0, 0.0, 0.0, 1.0 });
    nvgBeginPath(ui::vg);
    nvgRect(ui::vg, 0, 0, ui::view.width, ui::view.height);
    nvgFill(ui::vg);

    constexpr float HEADER_HEIGHT = 70.0;

    // ScrollView
    constexpr float BLOCK_HEIGHT = 80.0;
    {
        SubView sub(0, HEADER_HEIGHT, ui::view.width, ui::keyboard_y() - HEADER_HEIGHT);
        ScrollView sv(&sv_state);
        sv.inner_size(ui::view.width, data_layer::conversations.size() * BLOCK_HEIGHT).update();

        int start_block = (int)(sv.state.scroll_y / BLOCK_HEIGHT);
        if (start_block < 0) start_block = 0;
        int end_block = start_block + (int)(sv.outer_height / BLOCK_HEIGHT) + 1;

        for (int i = start_block; i <= end_block && i < data_layer::conversations.size(); ++i) {

            int conversation_id = data_layer::conversations_sorted[data_layer::conversations.size() - i - 1];
            auto& conv = data_layer::conversations[conversation_id];
            auto profile = data_layer::get_or_request_profile(&conv.counterparty);

            int y = i * BLOCK_HEIGHT;

            // Highlight if conversation is open
            if (Root::open_conversation() == conversation_id) {
                nvgBeginPath(ui::vg);
                nvgFillColor(ui::vg, ui::color(0x4D434B, 1.0 - Root::pop_transition_progress()));
                nvgRect(ui::vg, 0, y, ui::view.width, BLOCK_HEIGHT);
                nvgFill(ui::vg);
            }

            if (ui::simple_tap(0, y, ui::view.width, BLOCK_HEIGHT)) {
                Root::push_view_chat(conversation_id);
                return ui::redraw();
            }

            constexpr float PROFILE_PADDING = 10.0;
            {
                SubView sub(PROFILE_PADDING, y + PROFILE_PADDING, BLOCK_HEIGHT - 2.0 * PROFILE_PADDING, BLOCK_HEIGHT - 2.0 * PROFILE_PADDING);

                const char* image_url = NULL;
                if (profile && profile->picture.size) {
                    image_url = profile->picture.data.get(profile);
                }

                int image_id;
                if (image_url && (image_id = data_layer::get_image(image_url))) {
                    auto paint = nvgImagePattern(ui::vg, 0, 0, ui::view.width, ui::view.height, 0, image_id, 1);
                    nvgFillPaint(ui::vg, paint);
                } else if (data_layer::does_first_follow_second(&account.pubkey, &conv.counterparty)) {
                    nvgFillColor(ui::vg, (NVGcolor){ 0.7, 0.7, 0.7, 1.0 });
                } else {
                    nvgFillColor(ui::vg, (NVGcolor){ 0.5, 0.5, 0.5, 1.0 });
                }

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
            if (!profile || !profile->display_name.size) {
                NostrEntity::encode_npub(&conv.counterparty, name, NULL);
            } else {
                strcpy(name, profile->display_name.data.get(profile));
            }

            nvgText(ui::vg, BLOCK_HEIGHT, y + CONTENT_PADDING + CONTENT_HEIGHT * (1.0 / 6.0), name, NULL);

            auto event = conv.messages.back();

            const char* text;
            if (event->kind != 4) {
                text = event->content.data.get(event);
            } else if (event->content_encryption == EVENT_CONTENT_DECRYPTED) {
                text = event->content.data.get(event);
            } else if (event->content_encryption == EVENT_CONTENT_DECRYPT_FAILED) {
                text = "Failed to decrypt";
            } else {
                text = "Failed to decrypt";
            }

            ui::save();
            nvgScissor(ui::vg, 0, y, ui::view.width, BLOCK_HEIGHT - 6);
            nvgFillColor(ui::vg, ui::color(0xcccccc));
            nvgFontSize(ui::vg, 15.0);
            nvgFontFace(ui::vg, "regular");
            nvgTextLineHeight(ui::vg, 1.3);
            nvgTextAlign(ui::vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
            nvgTextBox(ui::vg, BLOCK_HEIGHT, y + 34, ui::view.width - BLOCK_HEIGHT - CONTENT_PADDING, text, NULL);
            ui::restore();

            nvgFillColor(ui::vg, (NVGcolor){ 0.2, 0.2, 0.2, 1.0 });
            nvgBeginPath(ui::vg);
            nvgRect(ui::vg, BLOCK_HEIGHT, y + BLOCK_HEIGHT, ui::view.width - BLOCK_HEIGHT, 0.5);
            nvgFill(ui::vg);
        }
    }

    // Header
    {
        SubView sub(0, 0, ui::view.width, HEADER_HEIGHT);
        nvgBeginPath(ui::vg);
        nvgRect(ui::vg, 0, 0, ui::view.width, ui::view.height);
        nvgFillColor(ui::vg, ui::color(0x4D434B));
        nvgFill(ui::vg);

        nvgBeginPath(ui::vg);
        nvgStrokeColor(ui::vg, ui::color(0x000000, 0.2));
        nvgMoveTo(ui::vg, 0, HEADER_HEIGHT - 0.5);
        nvgLineTo(ui::vg, ui::view.width, HEADER_HEIGHT - 0.5);
        nvgStroke(ui::vg);
    }
}
