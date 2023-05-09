//
//  events.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-30.
//

#include "events.hpp"
#include "conversations.hpp"
#include "accounts.hpp"
#include "profiles.hpp"
#include "contact_lists.hpp"
#include "../models/event_content.hpp"
#include "../models/nip31.hpp"
#include <app.hpp>
#include <vector>

#include "../models/event_stringify.hpp"

namespace data_layer {

std::vector<Event*> events;

static EventLocator store_event_by_copying(Event* event);
static EventLocator store_event_without_copying(Event* event);
static void handle_kind_4(Event* event);

void receive_event(Event* event, int32_t relay_id, uint64_t receipt_time) {

    // Have we already received this event?
    for (auto& event_other : events) {
        if (compare_keys(&event_other->id, &event->id)) {

            // Add/update receipt info
            bool has_receipt = false;
            for (auto& receipt : event_other->receipt_info.get(event_other)) {
                if (receipt.relay_id == relay_id) {
                    if (receipt.receipt_time < receipt_time) {
                        receipt.receipt_time = receipt_time;
                    }
                    has_receipt = true;
                    break;
                }
            }
            if (!has_receipt && event_other->receipt_info.can_push_back()) {
                ReceiptInfo receipt;
                receipt.relay_id = relay_id;
                receipt.receipt_time = receipt_time;
                event_other->receipt_info.push_back(event_other, receipt);
            }

            return;
        }
    }

    // Validate the event
    if (!event_validate(event)) {
        printf("event invalid: %s\n", event->validity == EVENT_INVALID_ID ? "INVALID_ID" : "INVALID_SIG");
        return;
    }

    // Add receipt info
    if (event->receipt_info.can_push_back()) {
        ReceiptInfo receipt;
        receipt.relay_id = relay_id;
        receipt.receipt_time = receipt_time;
        event->receipt_info.push_back(event, receipt);
    }

    switch (event->kind) {
        case 0: {
            auto event_loc = store_event_by_copying(event);
            data_layer::receive_profile(event_loc);
            break;
        }
        case 3: {
            auto event_loc = store_event_by_copying(event);
            data_layer::receive_contact_list(event_loc);
            break;
        }
        case 4: {
            handle_kind_4(event);
            break;
        }
        default: {
            printf("received event kind %d, which we don't handle current...\n", event->kind);
            break;
        }
    }
}

const Event* event(EventLocator event_loc) {
    if (event_loc < 0 || event_loc >= events.size()) {
        return NULL;
    }
    return events[event_loc];
}

EventLocator store_event_by_copying(Event* event_) {
    Event* event = (Event*)malloc(Event::size_of(event_));
    memcpy(event, event_, Event::size_of(event_));
    EventLocator event_loc = (int)events.size();
    events.push_back(event);
    return event_loc;
}

EventLocator store_event_without_copying(Event* event) {
    EventLocator event_loc = (int)events.size();
    events.push_back(event);
    return event_loc;
}

void handle_kind_4(Event* event) {

    auto account = data_layer::current_account();

    // Determine counterparty
    Pubkey counterparty;
    if (!event->p_tags.size) return;
    if (compare_keys(&event->p_tags.get(event, 0).pubkey, &account->pubkey)) {
        counterparty = event->pubkey;
    } else {
        counterparty = event->p_tags.get(event, 0).pubkey;
    }

    // Copy the event to the heap
    Event* event_copy = (Event*)malloc(Event::size_of(event));
    memcpy(event_copy, event, Event::size_of(event));

    // Decrypt the message
    event->content_encryption = EVENT_CONTENT_ENCRYPTED;
    auto ciphertext = event->content.data.get(event);
    auto len = event->content.size;
    account_nip04_decrypt(account, &counterparty, ciphertext, len,
        [event(event_copy), counterparty](bool error, const char* error_reason, const char* plaintext, uint32_t len) {

            // Get the result
            const char* result;
            if (error) {
                event->content_encryption = EVENT_CONTENT_DECRYPT_FAILED;
                auto event_loc = store_event_without_copying(event);
                receive_direct_message(event_loc);
                return;
            }

            // Copy over the decrypted data
            if (len > event->content.size) {
                printf("NIP04: Decoded plaintext longer than encoded ciphertext!!!\n");
                event->content_encryption = EVENT_CONTENT_DECRYPT_FAILED;
                auto event_loc = store_event_without_copying(event);
                receive_direct_message(event_loc);
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

            // Realloc the event to fit the tokenized contents
            auto event_2 = (Event*)realloc(event, event_content_size_needed_for_copy(event, tokens, entities));
            event_content_copy_result_into_event(event_2, tokens, entities);

            auto event_loc = store_event_without_copying(event_2);
            receive_direct_message(event_loc);

        }
    );

}

}
