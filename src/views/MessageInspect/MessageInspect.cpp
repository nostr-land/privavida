//
//  MessageInspect.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 11/05/2023.
//

#include "MessageInspect.hpp"
#include "../ChatView/ChatMessage.hpp"
#include "../SubView.hpp"
#include "../Root.hpp"
#include <app.hpp>

void MessageInspect::update(const Event* event) {
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
        nvgFillColor(ui::vg, ui::color(0x4D434B));
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

    // Message
    {
        auto cm = ChatMessage::create(event);
        auto height = cm->measure_height(ui::view.width, NULL, NULL);
        SubView sub(0, HEADER_HEIGHT, ui::view.width, height);
        cm->update();
    }
}
