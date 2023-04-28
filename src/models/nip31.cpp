//
//  nip31.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-28.
//

#include "nip31.hpp"
#include "event_compose.hpp"

bool nip31_verify_invite(NostrEntity* invite, const Pubkey* recipient) {

    EventDraft draft = { 0 };
    draft.pubkey = invite->pubkey;
    draft.kind = 0;
    draft.content = "";

    PTag p_tags[2];
    p_tags[0].pubkey = *recipient;
    p_tags[1].pubkey = *invite->invite_conversation_pubkey.get(invite);
    draft.p_tags = Array<PTag>(2, p_tags);

    uint8_t event_stack[event_compose_size(&draft)];
    Event* event = (Event*)event_stack;
    event_compose(event, &draft);

    event->created_at = 0;
    event_compute_hash(event, &event->id);

    event->sig = *invite->invite_signature.get(invite);

    bool valid = event_validate(event);
    invite->invite_signature_state = valid ? INVITE_SIGNATURE_VALID : INVITE_SIGNATURE_INVALID;

    return valid;
}
