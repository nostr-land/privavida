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
#include "../../models/nostr_entity.hpp"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void ChatView::update() {

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
    }

    auto& conv = data_layer::conversations[conversation_id];

    // Update messages
    if (conv.messages.size() != messages.size()) {
        messages.clear();
        messages.reserve(conv.messages.size());
        for (auto event : conv.messages) {
            messages.push_back(ChatMessage::create(event));
        }
    }

    // ScrollView
    {
        SubView sub(0, HEADER_HEIGHT, ui::view.width, ui::keyboard_y() - HEADER_HEIGHT - composer.height());
        VirtualizedList::update(
            &virt_state,
            (int)conv.messages.size(),
            [&](int i) {
                auto event_before = i - 1 >= 0 ? conv.messages[i - 1] : NULL;
                auto event_after  = i + 1 < conv.messages.size() ? conv.messages[i + 1] : NULL;
                return messages[i]->measure_height(ui::view.width, event_before, event_after);
            },
            [&](int i) {
                messages[i]->update();
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
