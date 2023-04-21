//
//  accounts.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-21.
//

#pragma once
#include "../models/event.hpp"

struct Account {

    enum Type : uint8_t {
        PUBKEY_ONLY = 0,
        SECKEY_IN_MEMORY = 1,
        SECKEY_ON_SECURE_DEVICE = 2
    };

    Type type;
    Pubkey pubkey;
};

struct AccountResponse {

    enum Action {
        SIGN_EVENT,
        NIP04_ENCRYPT,
        NIP04_DECRYPT
    };

    Action action;
    void* user_data;

    bool error;
    const char* error_reason;

    const char* result_nip04;
    uint32_t    result_nip04_len;

    const Event* result_sign_event;
};

typedef void (*AccountResponseCallback)(const AccountResponse* response);

extern int account_selected;
extern Array<Account*> accounts;
extern AccountResponseCallback account_response_callback;

bool accounts_load();
void accounts_process_responses();

void account_sign_event   (const Account* account, const Event* event, void* user_data);
void account_nip04_encrypt(const Account* account, const Pubkey* pubkey, const char* plaintext,  uint32_t len, void* user_data);
void account_nip04_decrypt(const Account* account, const Pubkey* pubkey, const char* ciphertext, uint32_t len, void* user_data);
