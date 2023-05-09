//
//  nip31.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-28.
//

#include "nip31.hpp"
#include "event_builder.hpp"

bool nip31_verify_invite(NostrEntity* invite, const Pubkey* recipient) {

    StackBufferFixed<256> stack_buffer;

    auto event = EventBuilder(&stack_buffer)
        .kind(0)
        .content("")
        .created_at(0)
        .pubkey(&invite->pubkey)
        .p_tag(recipient)
        .p_tag(invite->invite_conversation_pubkey.get(invite))
        .sig(invite->invite_signature.get(invite))
        .finish();
    
    event_compute_hash(event, &event->id);
    bool valid = event_validate(event);
    invite->invite_signature_state = valid ? INVITE_SIGNATURE_VALID : INVITE_SIGNATURE_INVALID;

    return valid;
}
