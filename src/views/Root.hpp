//
//  Root.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 10/05/2023.
//

#pragma once
#include "../models/event.hpp"

struct Root {
    static void init();
    static void update();

    static void push_view_chat(int conversation_id);
    static void push_view_message_inspect(const Event* event);
    static void pop_view();

    static int open_conversation();
    static double pop_transition_progress();
};
