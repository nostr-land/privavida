//
//  Root.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 10/05/2023.
//

#include "Root.hpp"
#include <app.hpp>
#include "Conversations.hpp"
#include "ChatView/ChatView.hpp"
#include "MessageInspect/MessageInspect.hpp"
#include "../data_layer/accounts.hpp"
#include "../utils/animation.hpp"

static const void* ANIMATION_PUSH = (void*)0x012921;
static const void* ANIMATION_POP = (void*)0x012922;
const double ANIMATION_PUSH_DURATION = 0.3;
const double ANIMATION_POP_DURATION = 0.3;

static std::vector<ChatView> chat_views;

struct ViewStackItem {
    enum Type {
        CONVERSATIONS,
        CHAT_VIEW,
        MESSAGE_INSPECT
    };

    Type type;
    int conversation_id;
    EventLocator event_loc;
};

enum TransitionState {
    STATE_IDLE,
    STATE_PUSHING,
    STATE_POPPING
};

static std::vector<ViewStackItem> view_stack;
static TransitionState transition_state = STATE_IDLE;

void Root::init() {
    if (!data_layer::current_account()) {
        // TODO: login flow??
        return;
    }

    Conversations::init();

    ViewStackItem root_view;
    root_view.type = ViewStackItem::CONVERSATIONS;
    view_stack.push_back(root_view);
}

static void update_stack_item(ViewStackItem& item) {
    switch (item.type) {
        case ViewStackItem::CONVERSATIONS: {
            Conversations::update();
            return;
        }
        case ViewStackItem::CHAT_VIEW: {
            for (auto& chat_view : chat_views) {
                if (chat_view.conversation_id == item.conversation_id) {
                    chat_view.update();
                    return;
                }
            }
            chat_views.push_back(ChatView());
            auto& chat_view = chat_views.back();
            chat_view.conversation_id = item.conversation_id;
            chat_view.update();
            return;
        }
        case ViewStackItem::MESSAGE_INSPECT: {
            MessageInspect::update(item.event_loc);
            return;
        }
    }
}

void Root::update() {
    if (view_stack.empty()) return;

    bool in_transition = false;
    float translate_upper_x, translate_lower_x;

    if (animation::is_animating(ANIMATION_PUSH)) {
        auto completion = animation::get_time_elapsed(ANIMATION_PUSH) / ANIMATION_PUSH_DURATION;
        if (completion > 1.0) {
            animation::stop(ANIMATION_PUSH);
        } else {
            in_transition = true;
            translate_upper_x = (1.0 - animation::ease_out(completion)) * ui::view.width;
        }
    } else if (animation::is_animating(ANIMATION_POP)) {
        auto completion = animation::get_time_elapsed(ANIMATION_POP) / ANIMATION_POP_DURATION;
        if (completion > 1.0) {
            animation::stop(ANIMATION_POP);
            view_stack.pop_back();
        } else {
            in_transition = true;
            translate_upper_x = animation::ease_out(completion) * ui::view.width;
        }
    }
    translate_lower_x = 0.5 * (translate_upper_x - ui::view.width);

    if (!in_transition) {
        update_stack_item(view_stack.back());
        return;
    }

    ui::save();
    nvgTranslate(ui::vg, translate_lower_x, 0);
    update_stack_item(view_stack[view_stack.size() - 2]);
    ui::restore();

    ui::save();
    nvgTranslate(ui::vg, translate_upper_x, 0);
    update_stack_item(view_stack[view_stack.size() - 1]);
    ui::restore();
}

void Root::push_view_chat(int conversation_id) {
    ViewStackItem item;
    item.type = ViewStackItem::CHAT_VIEW;
    item.conversation_id = conversation_id;
    view_stack.push_back(item);
    animation::start(ANIMATION_PUSH);
}

void Root::push_view_message_inspect(EventLocator event_loc) {
    ViewStackItem item;
    item.type = ViewStackItem::MESSAGE_INSPECT;
    item.event_loc = event_loc;
    view_stack.push_back(item);
    animation::start(ANIMATION_PUSH);
}

void Root::pop_view() {
    if (view_stack.size() > 1) {
        animation::start(ANIMATION_POP);
    }
}

int Root::open_conversation() {
    for (auto& item : view_stack) {
        if (item.type == ViewStackItem::CHAT_VIEW) {
            return item.conversation_id;
        }
    }
    return -1;
}

double Root::pop_transition_progress() {
    if (animation::is_animating(ANIMATION_POP)) {
        return animation::get_time_elapsed(ANIMATION_POP) / ANIMATION_POP_DURATION;
    } else {
        return 0.0;
    }
}
