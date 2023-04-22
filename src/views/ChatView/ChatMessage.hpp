//
//  ChatMessage.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-21.
//

#pragma once

#include "../../models/event.hpp"
#include "TokenizedContent.hpp"

struct ChatMessage {
    const Event* event;
    TokenizedContent::State tokenized_content;
    float content_width;
    bool space_above, space_below;

    static ChatMessage create(const Event* event);
    float measure_height(float width, const Event* event_before, const Event* event_after);
    void update();
};
