//
//  ChatView.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 11/05/2023.
//

#pragma once
#include "VirtualizedList.hpp"
#include "ChatViewEntry.hpp"
#include "Composer.hpp"

struct ChatView {
    int conversation_id;

    int selected_idx = 0;
    VirtualizedList::State virt_state;
    std::vector<ChatViewEntry*> entries;
    Composer composer;

    void update();
};
