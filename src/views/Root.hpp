//
//  Root.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 10/05/2023.
//

#pragma once
#include "../data_layer/events.hpp"

struct Root {
    static void init();
    static void update();

    static void push_view_chat(int conversation_id);
    static void push_view_message_inspect(EventLocator event);
    static void pop_view();

    static int open_conversation();
    static double pop_transition_progress();
};
