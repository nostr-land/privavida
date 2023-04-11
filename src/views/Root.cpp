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
    // for (int i = 0; i < num_lines; ++i) {
    //     lines[i] = (char*)malloc(max_line_len);
    //     snprintf(lines[i], max_line_len, "%d", i + 1);
    // }
}

void Root::update() {
    Conversations::update();
}
