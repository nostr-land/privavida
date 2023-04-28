//
//  ChatMessage.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-21.
//

#pragma once

#include "../../models/event.hpp"
#include "../TextRender/TextRender.hpp"

struct ChatMessage {
    const Event* event;
    StackArrayFixed<char, 100> text_content;
    StackArrayFixed<TextRender::Attribute, 4> text_attrs;
    StackArrayFixed<TextRender::Line, 16> text_lines;
    StackArrayFixed<TextRender::Run, 32>  text_runs;
    float content_width;
    bool space_above, space_below;

    static ChatMessage* create(const Event* event);
    float measure_height(float width, const Event* event_before, const Event* event_after);
    void update();
};
