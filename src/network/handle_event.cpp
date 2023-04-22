//
//  handle_event.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 21/07/2023.
//

#include "handle_event.hpp"
#include "../models/profile.hpp"
#include "../data_layer/accounts.hpp"
#include "../data_layer/conversations.hpp"
#include "../data_layer/profiles.hpp"
#include "../data_layer/contact_lists.hpp"
#include <string.h>
#include <stdio.h>
#include <algorithm>

bool sort_by_created_at(const Event* a, const Event* b) {
    return a->created_at < b->created_at;
}

static void handle_kind_0(const char* subscription_id, Event* event);
static void handle_kind_3(const char* subscription_id, Event* event);
static void handle_kind_4(const char* subscription_id, Event* event);

void network::handle_event(const char* subscription_id, Event* event_) {

    Event* event = (Event*)malloc(Event::size_of(event_));
    memcpy(event, event_, Event::size_of(event_));

    if (!event_validate(event)) {
        printf("event invalid: %s\n", event->validity == EVENT_INVALID_ID ? "INVALID_ID" : "INVALID_SIG");
        return;
    }

    if (event->kind == 0) {
        handle_kind_0(subscription_id, event);
    } else if (event->kind == 3) {
        handle_kind_3(subscription_id, event);
    } else if (event->kind == 4) {
        handle_kind_4(subscription_id, event);
    } else {
        printf("received event kind %d, which we don't handle current...\n", event->kind);
    }

}

static void handle_kind_0(const char* subscription_id, Event* event) {

    Profile* profile = (Profile*)malloc(Profile::size_from_event(event));

    if (!parse_profile_data(profile, event)) {
        printf("Invalid profile data :(\n");
        printf("%s\n", event->content.data.get(event));
        free(profile);
        return;
    }

    data_layer::profiles.push_back(profile);
    ui::redraw();
}

static void handle_kind_3(const char* subscription_id, Event* event) {

    // Add the contact list
    bool replaced_older = false;
    for (auto i = 0; i < data_layer::contact_lists.size(); ++i) {
        if (compare_keys(&data_layer::contact_lists[i]->pubkey, &event->pubkey)) {
            if (data_layer::contact_lists[i]->created_at > event->created_at) {
                return;
            }
            data_layer::contact_lists[i] = event;
            replaced_older = true;
            break;
        }
    }
    if (!replaced_older) {
        data_layer::contact_lists.push_back(event);
    }

    // Is it our contact list?
    if (compare_keys(&event->pubkey, &data_layer::accounts[data_layer::account_selected].pubkey)) {
        for (auto& p_tag : event->p_tags.get(event)) {
            data_layer::request_profile(&p_tag.pubkey);
        }
        data_layer::batch_profile_requests_send();
    }

    ui::redraw();
}

static void handle_kind_4(const char* subscription_id, Event* event) {

    auto& account = data_layer::accounts[data_layer::account_selected];

    // Process a NIP-04 direct message

    Pubkey counterparty;
    if (strcmp(subscription_id, "dms_sent") == 0) {
        if (!event->p_tags.size) return;
        counterparty = event->p_tags.get(event, 0).pubkey;
    } else if (strcmp(subscription_id, "dms_received") == 0) {
        counterparty = event->pubkey;
    } else {
        return;
    }

    // Find the conversation (or create it)
    int conversation_id = -1;
    for (int i = 0; i < data_layer::conversations.size(); ++i) {
        if (compare_keys(&data_layer::conversations[i].counterparty, &counterparty)) {
            conversation_id = i;
            break;
        }
    }
    if (conversation_id == -1) {
        data_layer::Conversation conv;
        conv.counterparty = counterparty;
        conversation_id = (int)data_layer::conversations.size();
        data_layer::conversations.push_back(conv);
    }

    // Add the message to the conversation
    auto& conv = data_layer::conversations[conversation_id];
    conv.messages.push_back(event);
    std::sort(conv.messages.begin(), conv.messages.end(), &sort_by_created_at);

    // Decrypt the message
    event->content_encryption = EVENT_CONTENT_ENCRYPTED;
    auto ciphertext = event->content.data.get(event);
    auto len = event->content.size;
    account_nip04_decrypt(&account, &counterparty, ciphertext, len, event);

    ui::redraw();
}

void network::account_response_handler(const AccountResponse* response) {
    switch (response->action) {
        case AccountResponse::SIGN_EVENT: {
            printf("signed event!\n");
            break;
        }
        case AccountResponse::NIP04_ENCRYPT: {
            break;
        }
        case AccountResponse::NIP04_DECRYPT: {
            Event* event = (Event*)response->user_data;

            // Get the result
            const char* result;
            if (response->error) {
                event->content_encryption = EVENT_CONTENT_DECRYPT_FAILED;
                return;
            }

            // Copy over the decrypted data
            auto decrypted = response->result_nip04;
            auto decrypted_len = response->result_nip04_len;
            if (decrypted_len > event->content.size) {
                printf("NIP04: Decoded plaintext longer than encoded ciphertext!!!\n");
                event->content_encryption = EVENT_CONTENT_DECRYPT_FAILED;
                return;
            }

            event->content_encryption = EVENT_CONTENT_DECRYPTED;
            memcpy(event->content.data.get(event), decrypted, decrypted_len);
            event->content.data.get(event)[decrypted_len] = '\0';
            event->content.size = decrypted_len;
            break;
        }
    }
}
