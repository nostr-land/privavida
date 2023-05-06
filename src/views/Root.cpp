//
//  Root.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 10/05/2023.
//

#include "Root.hpp"
#include <app.hpp>
#include "LoginView.hpp"
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
        LOGIN_VIEW,
        CONVERSATIONS,
        CHAT_VIEW,
        MESSAGE_INSPECT
    };

    Type type;
    int conversation_id;
    EventLocator event_loc;
};

static std::vector<ViewStackItem> view_stack;

void Root::init() {
    if (!data_layer::accounts_load()) return;

    Conversations::init();

    ViewStackItem root_view;
    root_view.type = ViewStackItem::CONVERSATIONS;
    view_stack.push_back(root_view);

    if (!data_layer::current_account()) {
        push_login_view();
    }
}

static void update_stack_item(ViewStackItem& item) {
    switch (item.type) {
        case ViewStackItem::LOGIN_VIEW: {
            LoginView::update();
            return;
        }
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

static bool update_transition(float* completion_out) {
    if (animation::is_animating(ANIMATION_PUSH)) {
        float completion = animation::get_time_elapsed(ANIMATION_PUSH) / ANIMATION_PUSH_DURATION;
        if (completion > 1.0) {
            animation::stop(ANIMATION_PUSH);
        } else {
            *completion_out = 1.0 - animation::ease_out(completion);
            return true;
        }
    } else if (animation::is_animating(ANIMATION_POP)) {
        auto completion = animation::get_time_elapsed(ANIMATION_POP) / ANIMATION_POP_DURATION;
        if (completion > 1.0) {
            animation::stop(ANIMATION_POP);
            view_stack.pop_back();
        } else {
            *completion_out = animation::ease_out(completion);
            return true;
        }
    }
    return false;
}

static void compute_transition(float completion, float* xform_front, float* xform_back) {
    nvgTransformIdentity(xform_front);
    nvgTransformIdentity(xform_back);
    
    if (view_stack.back().type == ViewStackItem::LOGIN_VIEW) {
        // Slide front view up from bottom
        nvgTransformTranslate(xform_front, 0, completion * ui::view.height);
    } else {
        // Slide view in from the right
        nvgTransformTranslate(xform_front, completion * ui::view.width, 0);
        nvgTransformTranslate(xform_back, - 0.5 * (1.0 - completion) * ui::view.width, 0);
    }
}

void Root::update() {
    if (view_stack.empty()) return;

    float completion;
    bool in_transition = update_transition(&completion);
    if (!in_transition) {
        update_stack_item(view_stack.back());
        return;
    }

    float a[6], b[6];
    compute_transition(completion, b, a);

    ui::save();
    nvgTransform(ui::vg, a[0], a[1], a[2], a[3], a[4], a[5]);
    update_stack_item(view_stack[view_stack.size() - 2]);
    ui::restore();

    ui::save();
    nvgTransform(ui::vg, b[0], b[1], b[2], b[3], b[4], b[5]);
    update_stack_item(view_stack[view_stack.size() - 1]);
    ui::restore();
}

void Root::push_login_view() {
    ViewStackItem item;
    item.type = ViewStackItem::LOGIN_VIEW;
    view_stack.push_back(item);
    animation::start(ANIMATION_PUSH);
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
