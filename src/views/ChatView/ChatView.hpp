//
//  ChatView.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 11/05/2023.
//

#pragma once
#include "VirtualizedList.hpp"
#include "ChatMessage.hpp"
#include "Composer.hpp"

struct ChatView {
    int conversation_id;

    int selected_idx = 0;
    VirtualizedList::State virt_state;
    std::vector<ChatMessage*> messages;
    Composer composer;

    void update();
};
