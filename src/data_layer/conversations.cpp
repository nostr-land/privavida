//
//  conversations.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-21.
//

#include "conversations.hpp"
#include "accounts.hpp"
#include "../models/event_compose.hpp"
#include "../models/event_stringify.hpp"
#include "../models/event_content.hpp"
#include "../network/network.hpp"
#include <stdio.h>
#include <vector>
#include <algorithm>

namespace data_layer {

std::vector<Conversation> conversations;
std::vector<int> conversations_sorted;

static bool sort_by_created_at(const Event* a, const Event* b) {
    return a->created_at < b->created_at;
}

static bool sort_conv_by_created_at(int a, int b) {
    return conversations[a].messages.back()->created_at < conversations[b].messages.back()->created_at;
}

static void receive_direct_message_2(Event* event, Pubkey counterparty) {

    // Find the conversation (or create it)
    int conversation_id = -1;
    for (int i = 0; i < conversations.size(); ++i) {
        if (compare_keys(&conversations[i].counterparty, &counterparty)) {
            conversation_id = i;
            break;
        }
    }
    if (conversation_id == -1) {
        data_layer::Conversation conv;
        conv.counterparty = counterparty;
        conversation_id = (int)conversations.size();
        conversations.push_back(conv);
        conversations_sorted.push_back(conversation_id);
    }

    // Add the message to the conversation
    auto& conv = conversations[conversation_id];
    for (auto other_event : conv.messages) {
        if (compare_keys(&other_event->id, &event->id)) {
            return; // Already added
            // @WARNING Leaking event here, should de-alloc? What's the ownership rules?
        }
    }
    conv.messages.push_back(event);
    std::sort(conv.messages.begin(), conv.messages.end(), &sort_by_created_at);
    std::sort(conversations_sorted.begin(), conversations_sorted.end(), &sort_conv_by_created_at);

    ui::redraw();
}

void receive_direct_message(Event* event) {

    auto& account = data_layer::accounts[data_layer::account_selected];

    // Determine counterparty
    Pubkey counterparty;
    if (!event->p_tags.size) return;
    if (compare_keys(&event->p_tags.get(event, 0).pubkey, &account.pubkey)) {
        counterparty = event->pubkey;
    } else {
        counterparty = event->p_tags.get(event, 0).pubkey;
    }

    // Decrypt the message
    event->content_encryption = EVENT_CONTENT_ENCRYPTED;
    auto ciphertext = event->content.data.get(event);
    auto len = event->content.size;
    account_nip04_decrypt(&account, &counterparty, ciphertext, len,
        [event, counterparty](bool error, const char* error_reason, const char* plaintext, uint32_t len) {

            // Get the result
            const char* result;
            if (error) {
                event->content_encryption = EVENT_CONTENT_DECRYPT_FAILED;
                receive_direct_message_2(event, counterparty);
                return;
            }

            // Copy over the decrypted data
            if (len > event->content.size) {
                printf("NIP04: Decoded plaintext longer than encoded ciphertext!!!\n");
                event->content_encryption = EVENT_CONTENT_DECRYPT_FAILED;
                receive_direct_message_2(event, counterparty);
                return;
            }

            // Copy decrypted content over into our event
            event->content_encryption = EVENT_CONTENT_DECRYPTED;
            memcpy(event->content.data.get(event), plaintext, len);
            event->content.data.get(event)[len] = '\0';
            event->content.size = len;

            // Parse/tokenize the content
            StackArrayFixed<EventContentToken, 10> tokens;
            StackArrayFixed<NostrEntity*, 10> entities;
            StackBufferFixed<1024> data;
            event_content_parse(event, tokens, entities, data);

            // Resize the event to allow the content to fit
            auto event_2 = (Event*)realloc(event, event_content_size_needed_for_copy(event, tokens, entities));
            event_content_copy_result_into_event(event_2, tokens, entities);

            receive_direct_message_2(event_2, counterparty);

        }
    );

}

static void send_direct_message_2(int conversation_id, const char* ciphertext) {

    auto& account = data_layer::accounts[data_layer::account_selected];
    auto& conv = conversations[conversation_id];

    EventDraft draft = { 0 };
    draft.pubkey = account.pubkey;
    draft.kind = 4;
    draft.content = ciphertext;

    PTag p_tags[1];
    p_tags[0].pubkey = conv.counterparty;
    draft.p_tags.data = p_tags;
    draft.p_tags.size = 1;

    uint8_t event_buffer[event_compose_size(&draft)];
    auto event = (Event*)event_buffer;
    event_compose(event, &draft);

    account_sign_event(&account, event, [](bool error, const char* error_reason, const Event* signed_event) {

        if (error) return;

        char out_event[1024];
        event_stringify(signed_event, out_event);
        char out_relay_message[1024];
        snprintf(out_relay_message, sizeof(out_relay_message), "[\"EVENT\",%s]", out_event);

        network::send(out_relay_message);
    });

}

void send_direct_message(int conversation_id, const char* message_text) {

    auto& account = data_layer::accounts[data_layer::account_selected];
    auto& conv = conversations[conversation_id];

    account_nip04_encrypt(&account, &conv.counterparty, message_text, (uint32_t)strlen(message_text),
        [conversation_id](bool error, const char* error_reason, const char* ciphertext, uint32_t len) {
            if (!error) {
                send_direct_message_2(conversation_id, ciphertext);
            }
        }
    );
}

}
