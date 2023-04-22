//
//  contact_lists.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-22.
//

#include "contact_lists.hpp"

namespace data_layer {

std::vector<Event*> contact_lists;

const Event* get_contact_list(const Pubkey* pubkey) {
    for (auto list : contact_lists) {
        if (compare_keys(&list->pubkey, pubkey)) {
            return list;
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
