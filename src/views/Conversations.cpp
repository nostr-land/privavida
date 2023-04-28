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
//#include "TokenizedContent/TokenizedContent.hpp"
#include "TextRender/TextRender.hpp"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static ScrollView::State sv_state;

void Conversations::init() {
}

void Conversations::update() {

    auto& account = data_layer::accounts[data_layer::account_selected];
    
    // Background
    nvgFillColor(ui::vg, COLOR_BACKGROUND);
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
                auto color = COLOR_HEADER;
                color.a = 1.0 - Root::pop_transition_progress();
                nvgFillColor(ui::vg, color);
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
                } else {
                    nvgFillColor(ui::vg, (NVGcolor){ 0.5, 0.5, 0.5, 1.0 });
                }

                nvgBeginPath(ui::vg);
                nvgCircle(ui::vg, 0.5 * ui::view.width, 0.5 * ui::view.width, 0.5 * ui::view.width);
                nvgFill(ui::vg);
            }

            constexpr float CONTENT_PADDING = 10.0;
            float CONTENT_HEIGHT = BLOCK_HEIGHT - 2 * CONTENT_PADDING;

            ui::text_align(NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgFillColor(ui::vg, (NVGcolor){ 1.0, 1.0, 1.0, 1.0 });
            ui::font_size(16.0);
            ui::font_face("bold");

            char name[100];
            if (!profile || !profile->display_name.size) {
                char npub[100];
                uint32_t len;
                NostrEntity::encode_npub(&conv.counterparty, npub, &len);
                npub[12] = '\0';
                snprintf(name, sizeof(name), "%s:%s", &npub[0], &npub[len - 8]);
            } else {
                strcpy(name, profile->display_name.data.get(profile));
            }

            ui::text(BLOCK_HEIGHT, y + CONTENT_PADDING + CONTENT_HEIGHT * (1.0 / 6.0), name, NULL);

            auto event = conv.messages.back();
            auto sent_by_me = compare_keys(&event->pubkey, &account.pubkey);

            char text[200];
            bool err = false;
            if (event->content_encryption == EVENT_CONTENT_DECRYPTED) {
                if (!sent_by_me) {
                    strncpy(text, event->content.data.get(event), sizeof(text) - 1);
                } else {
                    snprintf(text, sizeof(text) - 1, "You: %s", event->content.data.get(event));
                }
            } else if (event->content_encryption == EVENT_CONTENT_DECRYPT_FAILED) {
                err = true;
                strcpy(text, "Failed to decrypt");
            } else {
                err = true;
                strcpy(text, "Unknown data");
            }

            {
                SubView sv(BLOCK_HEIGHT, y + 36, ui::view.width - BLOCK_HEIGHT - CONTENT_PADDING, BLOCK_HEIGHT - 36);

                TextRender::Props props;
                props.data = Array<const char>((int)strlen(text), text);
                props.bounding_width = ui::view.width;
                props.bounding_height = ui::view.height;

                TextRender::Attribute attr[2];
                attr[0].index = 0;
                attr[0].font_face = "regular";
                attr[0].font_size = 15.0;
                attr[0].text_color = ui::color(0xcccccc);
                attr[0].line_spacing = 5.0;

                attr[1] = attr[0];
                attr[1].index = 5;

                if (sent_by_me) {
                    attr[0].text_color = ui::color(0xdddddd, 0.8);
                    props.attributes = Array<TextRender::Attribute>(2, attr);
                } else if (err) {
                    attr[0].text_color = COLOR_ERROR;
                    props.attributes = Array<TextRender::Attribute>(1, attr);
                } else {
                    props.attributes = Array<TextRender::Attribute>(1, attr);
                }

                TextRender::StateFixed state;
                TextRender::layout(&state, &props);
                TextRender::render(&state);
            }

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
        nvgFillColor(ui::vg, COLOR_HEADER);
        nvgFill(ui::vg);

        nvgBeginPath(ui::vg);
        nvgStrokeColor(ui::vg, ui::color(0x000000, 0.2));
        nvgMoveTo(ui::vg, 0, HEADER_HEIGHT - 0.5);
        nvgLineTo(ui::vg, ui::view.width, HEADER_HEIGHT - 0.5);
        nvgStroke(ui::vg);
    }
}
