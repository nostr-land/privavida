//
//  ChatView.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 11/05/2023.
//

#include "ChatView.hpp"
#include "Composer.hpp"
#include "../ScrollView.hpp"
#include "../SubView.hpp"
#include "../Root.hpp"
#include "../../data_layer/conversations.hpp"
#include "../../data_layer/accounts.hpp"
#include "../../data_layer/profiles.hpp"
#include "../../data_layer/images.hpp"
#include "../../models/nostr_entity.hpp"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void ChatView::update() {

    auto& conv = data_layer::conversations[conversation_id];
    auto profile = data_layer::get_or_request_profile(&conv.counterparty);

    // Background
    nvgFillColor(ui::vg, COLOR_BACKGROUND);
    nvgBeginPath(ui::vg);
    nvgRect(ui::vg, 0, 0, ui::view.width, ui::view.height);
    nvgFill(ui::vg);

    // Header
    constexpr float HEADER_HEIGHT = 70.0;
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

        if (ui::simple_tap(0, 0, ui::view.width, ui::view.height)) {
            Root::pop_view();
            ui::redraw();
        }

        // Profile picture
        constexpr float STATUS_BAR = 14.0;
        constexpr float PROFILE_PADDING = 8.0;
        {
            SubView sub(0, STATUS_BAR + PROFILE_PADDING, ui::view.width, ui::view.height - 2.0 * PROFILE_PADDING - STATUS_BAR);
            SubView sv(0.5 * (ui::view.width - ui::view.height), 0, ui::view.height, ui::view.height);

            const char* image_url = NULL;
            if (profile && profile->picture.size) {
                image_url = profile->picture.data.get(profile);
            }

            int image_id;
            if (image_url && (image_id = data_layer::get_image(image_url))) {
                auto paint = nvgImagePattern(ui::vg, 0, 0, ui::view.height, ui::view.height, 0, image_id, 1);
                nvgFillPaint(ui::vg, paint);
            } else {
                nvgFillColor(ui::vg, (NVGcolor){ 0.5, 0.5, 0.5, 1.0 });
            }

            nvgBeginPath(ui::vg);
            nvgCircle(ui::vg, 0.5 * ui::view.height, 0.5 * ui::view.height, 0.5 * ui::view.height);
            nvgFill(ui::vg);
        }
    }

    // Update entries
    if (conv.messages.size() != entries.size()) {
        entries.clear();
        entries.reserve(conv.messages.size());
        for (auto& message : conv.messages) {
            entries.push_back(ChatViewEntry::create(&message));
        }
    }

    // ScrollView
    {
        SubView sub(0, HEADER_HEIGHT, ui::view.width, ui::keyboard_y() - HEADER_HEIGHT - composer.height());
        VirtualizedList::update(
            &virt_state,
            (int)entries.size(),
            [&](int i) {
                auto entry_before = i - 1 >= 0 ? entries[i - 1] : NULL;
                auto entry_after  = i + 1 < entries.size() ? entries[i + 1] : NULL;
                return entries[i]->measure_height(ui::view.width, entry_before, entry_after);
            },
            [&](int i) {
                entries[i]->update();
            },
            []() {}
        );
    }

    // Composer
    {
        SubView sub(0, ui::keyboard_y() - composer.height(), ui::view.width, composer.height());
        auto message = composer.update();
        if (message) {
            data_layer::send_direct_message(conversation_id, message);
        }
    }

}
