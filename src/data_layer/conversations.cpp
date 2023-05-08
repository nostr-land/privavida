//
//  conversations.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-21.
//

#include "conversations.hpp"
#include "accounts.hpp"
#include "relays.hpp"
#include "../models/event_compose.hpp"
#include "../models/event_stringify.hpp"
#include "../models/event_content.hpp"
#include "../models/nip31.hpp"
#include "../network/network.hpp"
#include <stdio.h>
#include <vector>
#include <algorithm>

namespace data_layer {

std::vector<Conversation> conversations;
std::vector<int> conversations_sorted;

static bool sort_by_created_at(const Message& a, const Message& b) {
    return data_layer::event(a.event_loc)->created_at < data_layer::event(b.event_loc)->created_at;
}

static bool sort_conv_by_last_active_time(int a, int b) {
    return conversations[a].last_active_time < conversations[b].last_active_time;
}

static Conversation* get_or_create_conversation(Pubkey counterparty) {
    for (int i = 0; i < conversations.size(); ++i) {
        if (compare_keys(&conversations[i].counterparty, &counterparty)) {
            return &conversations[i];
        }
    }

    conversations.push_back(data_layer::Conversation());
    auto& conv = conversations.back();
    conv.counterparty = counterparty;

    int conversation_id = (int)conversations.size() - 1;
    conversations_sorted.push_back(conversation_id);
    return &conv;
}

void receive_direct_message(EventLocator event_loc) {

    auto event = data_layer::event(event_loc);

    Conversation* conv;
    Message message;
    message.event_loc = event_loc;

    // Does the message content contain an invite?
    const NostrEntity* invite = NULL;
    for (auto& token : event->content_tokens.get(event)) {
        if (token.type == EventContentToken::ENTITY &&
            token.entity.get(event)->type == NostrEntity::NINVITE) {
            invite = token.entity.get(event);
        }
    }
    if (invite && nip31_verify_invite(const_cast<NostrEntity*>(invite), &event->p_tags.get(event, 0).pubkey)) {
        message.type = Message::INVITE;
        conv = get_or_create_conversation(invite->pubkey);

        Pubkey alias = *invite->invite_conversation_pubkey.get(invite);

        bool alias_already_known = false;
        for (auto& other_alias : conv->aliases) {
            if (compare_keys(&alias, &other_alias)) {
                alias_already_known = true;
                break;
            }
        }
        if (alias_already_known) return; // Don't need this invite message

        conv->aliases.push_back(*invite->invite_conversation_pubkey.get(invite));

    } else {
        message.type = Message::DIRECT_MESSAGE;
        if (!event->p_tags.size) return; // Malformed direct message

        Pubkey counterparty;
        if (compare_keys(&event->pubkey, &data_layer::current_account()->pubkey)) {
            counterparty = event->p_tags.get(event, 0).pubkey;
        } else {
            counterparty = event->pubkey;
        }
        conv = get_or_create_conversation(counterparty);
    }

    // Add the message to the conversation
    conv->messages.push_back(message);
    std::sort(conv->messages.begin(), conv->messages.end(), &sort_by_created_at);
    conv->last_active_time = data_layer::event(conv->messages.back().event_loc)->created_at;
    std::sort(conversations_sorted.begin(), conversations_sorted.end(), &sort_conv_by_last_active_time);

    ui::redraw();
}

static void send_direct_message_2(int conversation_id, const char* ciphertext);

void send_direct_message(int conversation_id, const char* message_text) {

    auto account = data_layer::current_account();
    auto& conv = conversations[conversation_id];

    account_nip04_encrypt(account, &conv.counterparty, message_text, (uint32_t)strlen(message_text),
        [conversation_id](bool error, const char* error_reason, const char* ciphertext, uint32_t len) {
            if (!error) {
                send_direct_message_2(conversation_id, ciphertext);
            }
        }
    );
}

void send_direct_message_2(int conversation_id, const char* ciphertext) {

    auto account = data_layer::current_account();
    auto& conv = conversations[conversation_id];

    EventDraft draft = { 0 };
    draft.pubkey = account->pubkey;
    draft.kind = 4;
    draft.content = ciphertext;

    PTag p_tags[1];
    p_tags[0].pubkey = conv.counterparty;
    draft.p_tags.data = p_tags;
    draft.p_tags.size = 1;

    uint8_t event_buffer[event_compose_size(&draft)];
    auto event = (Event*)event_buffer;
    event_compose(event, &draft);

    account_sign_event(account, event, [](bool error, const char* error_reason, const Event* signed_event) {
        
        if (error) return;
        
        for (auto relay_id : get_default_relays()) {
            network::relay_add_task_publish(relay_id, signed_event);
        }
    });

}

}

