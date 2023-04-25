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
#include "../network/network.hpp"
#include <stdio.h>
#include <vector>

namespace data_layer {

std::vector<Conversation> conversations;

static void send_direct_message_2(int conversation_id, const char* ciphertext) {

    auto& account = data_layer::accounts[data_layer::account_selected];
    auto& conv = data_layer::conversations[conversation_id];

    EventTemplate temp = { 0 };
    temp.pubkey = account.pubkey;
    temp.kind = 4;
    temp.content = ciphertext;

    PTag p_tags[1];
    p_tags[0].pubkey = conv.counterparty;
    temp.p_tags.data = p_tags;
    temp.p_tags.size = 1;

    uint8_t event_buffer[event_compose_size(&temp)];
    auto event = (Event*)event_buffer;
    event_compose(event, &temp);

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
    auto& conv = data_layer::conversations[conversation_id];

    account_nip04_encrypt(&account, &conv.counterparty, message_text, (uint32_t)strlen(message_text),
        [conversation_id](bool error, const char* error_reason, const char* ciphertext, uint32_t len) {
            if (!error) {
                send_direct_message_2(conversation_id, ciphertext);
            }
        }
    );
}

}
