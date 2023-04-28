//
//  event_compose.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-25.
//

#pragma once
#include "event.hpp"

struct EventDraft {
    Pubkey      pubkey;
    uint32_t    kind;
    const char* content;
    Array<Array<const char*>> tags_other;
    Array<PTag> p_tags;
    Array<ETag> e_tags;
};

uint32_t event_compose_size(const EventDraft* draft);

void event_compose(Event* event, const EventDraft* draft);
