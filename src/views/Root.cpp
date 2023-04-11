//
//  Root.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 10/05/2023.
//

#include "Root.hpp"
#include "Conversations.hpp"
#include "../ui.hpp"

void Root::init() {
    Conversations::init();
}

void Root::update() {
    Conversations::update();
}
