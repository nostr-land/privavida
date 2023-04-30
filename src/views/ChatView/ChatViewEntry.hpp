//
//  ChatViewEntry.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-29.
//

#pragma once
#include "../../data_layer/conversations.hpp"

struct ChatViewEntry {

    data_layer::Message::Type type;
    bool space_above, space_below;

    static ChatViewEntry* create(const data_layer::Message* message);
    float measure_height(float width, const ChatViewEntry* entry_before, const ChatViewEntry* entry_after);
    void update();

};
