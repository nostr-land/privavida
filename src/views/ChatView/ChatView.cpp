//
//  ChatView.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 11/05/2023.
//

#include "ChatView.hpp"
#include "../ScrollView.hpp"
#include "../SubView.hpp"
#include "../Root.hpp"
#include "../../data_layer/conversations.hpp"
#include "../../data_layer/accounts.hpp"
#include "../../models/nostr_entity.hpp"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void ChatView::update() {

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
        nvgFillColor(ui::vg, (NVGcolor){ 0.1, 0.2, 0.1, 1.0 });
        nvgFill(ui::vg);

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
    constexpr float BLOCK_HEIGHT = 80.0;

    SubView sub(0, HEADER_HEIGHT, ui::view.width, kb_y - HEADER_HEIGHT);
    VirtualizedList::update(
        &virt_state,
        conv.messages.size(),
        [&](int i) {
            auto event_before = i - 1 >= 0 ? conv.messages[i - 1] : NULL;
            auto event_after  = i + 1 < conv.messages.size() ? conv.messages[i + 1] : NULL;
            return messages[i].measure_height(ui::view.width, event_before, event_after);
        },
        [&](int i) {
            messages[i].update();
        },
        []() {}
    );

}
