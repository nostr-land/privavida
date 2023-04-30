//
//  contact_lists.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-22.
//

#include "contact_lists.hpp"
#include "profiles.hpp"
#include "accounts.hpp"
#include <vector>
#include <app.hpp>

namespace data_layer {

std::vector<EventLocator> contact_lists;

void receive_contact_list(EventLocator event_loc) {

    auto event = data_layer::event(event_loc);

    // Add the contact list
    bool replaced_older = false;
    for (auto i = 0; i < contact_lists.size(); ++i) {
        auto other_event = data_layer::event(contact_lists[i]);
        if (compare_keys(&other_event->pubkey, &event->pubkey)) {
            if (other_event->created_at > event->created_at) {
                return;
            }
            contact_lists[i] = event_loc;
            replaced_older = true;
            break;
        }
    }
    if (!replaced_older) {
        contact_lists.push_back(event_loc);
    }

    // Is it our contact list?
    if (compare_keys(&event->pubkey, &current_account()->pubkey)) {
        auto p_tags = event->p_tags.get(event);
        for (auto& p_tag : p_tags) {
            request_profile(&p_tag.pubkey);
        }
        batch_profile_requests_send();
    }

    ui::redraw();
}

const Event* get_contact_list(const Pubkey* pubkey) {
    for (auto event_loc : contact_lists) {
        auto event = data_layer::event(event_loc);
        if (compare_keys(&event->pubkey, pubkey)) {
            return event;
        }
    }
    return NULL;
}

bool does_first_follow_second(const Pubkey* first, const Pubkey* second) {
    auto list = get_contact_list(first);
    if (!list) return false;

    auto p_tags = list->p_tags.get(list);
    for (const auto& p_tag : p_tags) {
        if (compare_keys(&p_tag.pubkey, second)) {
            return true;
        }
    }

    return false;
}

}
