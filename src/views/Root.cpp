//
//  Root.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 10/05/2023.
//

#include "Root.hpp"
#include "Conversations.hpp"
#include "Conversation.hpp"
#include "../ui.hpp"

int Root::open_conversation = -1;

void Root::init() {
    Conversations::init();
    Conversation::init();
}

void Root::update() {
    if (open_conversation == -1) {
        Conversations::update();
    } else {
        Conversation::update(open_conversation);
    }
}
