//
//  account.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-21.
//

#pragma once
#include "event.hpp"
#include <functional>

struct Account {

    enum Type : uint8_t {
        PUBKEY_ONLY = 0,
        SECKEY_IN_MEMORY = 1,
        SECKEY_ON_SECURE_DEVICE = 2
    };

    Type type;
    Pubkey pubkey;
    Seckey seckey_padded; // for Account::SECKEY_IN_MEMORY
};

bool account_load_from_file(Account* account, const uint8_t* data, uint32_t len);

typedef std::function<void(bool error, const char* error_reason, const Event* event)> AccountSignEventCallback;
typedef std::function<void(bool error, const char* error_reason, const char* ciphertext, uint32_t len)> AccountNIP04EncryptCallback;
typedef std::function<void(bool error, const char* error_reason, const char* plaintext, uint32_t len)> AccountNIP04DecryptCallback;

void account_sign_event   (const Account* account, const Event* event, AccountSignEventCallback cb);
void account_nip04_encrypt(const Account* account, const Pubkey* pubkey, const char* plaintext,  uint32_t len, AccountNIP04EncryptCallback cb);
void account_nip04_decrypt(const Account* account, const Pubkey* pubkey, const char* ciphertext, uint32_t len, AccountNIP04DecryptCallback cb);
