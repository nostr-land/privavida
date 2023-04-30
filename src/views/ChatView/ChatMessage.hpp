//
//  ChatMessage.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-21.
//

#pragma once

#include "../../data_layer/events.hpp"
#include "../TextRender/TextRender.hpp"

struct ChatMessage {
    EventLocator event_loc;
    StackArrayFixed<char, 100> text_content;
    StackArrayFixed<TextRender::Attribute, 4> text_attrs;
    StackArrayFixed<TextRender::Line, 16> text_lines;
    StackArrayFixed<TextRender::Run, 32>  text_runs;
    float content_width;

    static void create(ChatMessage* message, EventLocator event_loc);
    void measure_size(float width_available, float* width, float* height);
    void update(bool draw_bubble_tip);
};
