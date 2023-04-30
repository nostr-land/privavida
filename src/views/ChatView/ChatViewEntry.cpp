//
//  ChatViewEntry.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-29.
//

#include "ChatViewEntry.hpp"
#include "ChatMessage.hpp"
#include "../SubView.hpp"
#include "../../utils/icons.hpp"

constexpr auto SPACING = 10;
constexpr auto HORIZONTAL_PADDING = 14;
constexpr auto VERTICAL_PADDING = 1;

static bool a_moment_passed(const Event* earlier, const Event* later) {
    return (later->created_at - earlier->created_at) > 5 * 60; // 5 minutes
}

struct ChatViewEntryMessage : public ChatViewEntry {
    ChatMessage message;
};

ChatViewEntry* ChatViewEntry::create(const data_layer::Message* message) {
    if (message->type == data_layer::Message::DIRECT_MESSAGE) {
        auto entry = new ChatViewEntryMessage;
        entry->type = message->type;
        ChatMessage::create(&entry->message, message->event_loc);
        return entry;
    } else {
        auto entry = new ChatViewEntry;
        entry->type = message->type;
        return entry;
    }
}

float ChatViewEntry::measure_height(float width, const ChatViewEntry* entry_before, const ChatViewEntry* entry_after) {
    
    // We add spacing above if this is the first entry
    space_above = !entry_before;

    float inner_height;

    if (type == data_layer::Message::DIRECT_MESSAGE) {

        auto entry = (ChatViewEntryMessage*)this;
        auto event = data_layer::event(entry->message.event_loc);

        // Do we add spacing below this entry?
        if (!entry_after || entry_after->type != data_layer::Message::DIRECT_MESSAGE) {
            space_below = true;
        } else {
            auto event_loc_after = ((ChatViewEntryMessage*)entry_after)->message.event_loc;
            auto event_after = data_layer::event(event_loc_after);

            if (!compare_keys(&event->pubkey, &event_after->pubkey) ||
                a_moment_passed(event, event_after)) {
                space_below = true;
            } else {
                space_below = false;
            }
        }

        float inner_width;
        entry->message.measure_size(width - 2 * HORIZONTAL_PADDING, &inner_width, &inner_height);

    } else {

        space_below = true;
        inner_height = 50.0;

    }

    return inner_height + (space_above ? SPACING : 0) + (space_below ? SPACING : 0) + VERTICAL_PADDING;
}

void ChatViewEntry::update() {

    float inner_x = HORIZONTAL_PADDING;
    float inner_y = space_above ? SPACING : 0;
    float inner_w = ui::view.width - inner_x - HORIZONTAL_PADDING;
    float inner_h = ui::view.height - inner_y - VERTICAL_PADDING - (space_below ? SPACING : 0);
    SubView sv(inner_x, inner_y, inner_w, inner_h);

    if (type == data_layer::Message::DIRECT_MESSAGE) {
        ((ChatViewEntryMessage*)this)->message.update(space_below);
    } else {
        nvgBeginPath(ui::vg);
        nvgFillColor(ui::vg, COLOR_PRIMARY);
        ui::font_face("icons");
        ui::font_size(20.0);
        ui::text_align(NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        ui::text(0.5 * ui::view.width, 0.5 * ui::view.height, ui::ICON_LOCK, NULL);
    }
}
