//
//  account.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-21.
//

#include "account.hpp"
#include "../models/nip04.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void seckey_pad(const Seckey* seckey_in, Seckey* seckey_out) {
    auto ints_in  = (const uint64_t*)seckey_in->data;
    auto ints_out = (uint64_t*)seckey_out->data;

    // We've hardcoded a pad into the program. This is just to
    // prevent seckey being stored in plaintext on disk.
    // TODO: find a better method

    const uint8_t PAD[32] = {
        0x03, 0x2b, 0x8a, 0xcc, 0x87, 0x4d, 0x01, 0x85,
        0x01, 0x80, 0x3e, 0x37, 0x29, 0x9e, 0x40, 0x1b,
        0xfa, 0xb5, 0x88, 0xb6, 0x1e, 0x65, 0x5f, 0xcd,
        0x3f, 0xdc, 0x5e, 0x0e, 0x76, 0x4d, 0x18, 0xe0
    };
    for (int i = 0; i < sizeof(Seckey); ++i) {
        seckey_out->data[i] = PAD[i] ^ seckey_in->data[i];
    }
}

bool account_load_from_file(Account* account, const uint8_t* data, uint32_t len) {

    // First byte of the file is the account type
    if (len == 0) return false;
    account->type = (Account::Type)data[0];
    data++;
    len--;

    switch (account->type) {
        case Account::PUBKEY_ONLY: {

            // Pull pubkey from file
            if (len != sizeof(Pubkey)) {
                return false;
            }
            memcpy(account->pubkey.data, data, len);

            return true;
        }

        case Account::SECKEY_IN_MEMORY: {

            // Pull seckey_padded from file
            if (len != sizeof(Seckey)) {
                return false;
            }
            memcpy(account->seckey_padded.data, data, len);

            // Unpad to compute the pubkey
            Seckey seckey;
            seckey_pad(&account->seckey_padded, &seckey);
            auto res = get_public_key(&seckey, &account->pubkey);
            memset(&seckey, 0, sizeof(Seckey));

            // Failed to compute the pubkey
            if (!res) {
                printf("Failed to compute pubkey from file.\n");
                return false;
            }

            return true;
        }

        case Account::SECKEY_ON_SECURE_DEVICE: {

            printf("Seckey on secure device not supported yet.\n");
            return NULL;
        }

        default: {
            printf("Invalid account type.\n");
            return NULL;
        }
    }
}

bool account_store_to_file(const Account* account, FILE* fp) {

    // First byte of the file is the account type
    if (!fwrite(&account->type, 1, 1, fp)) return false;

    switch (account->type) {
        case Account::PUBKEY_ONLY: {
            // Write the pubkey
            if (!fwrite(&account->pubkey.data, sizeof(Pubkey), 1, fp)) return false;
            return true;
        }

        case Account::SECKEY_IN_MEMORY: {
            // Write the padded seckey
            if (!fwrite(&account->seckey_padded.data, sizeof(Seckey), 1, fp)) return false;
            return true;
        }

        case Account::SECKEY_ON_SECURE_DEVICE: {
            return true;
        }

        default: {
            return false;
        }
    }
}

bool account_from_pubkey(Account* account, const Pubkey* pubkey) {
    account->type = Account::PUBKEY_ONLY;
    memcpy(&account->pubkey, pubkey, sizeof(Pubkey));
    return true;
}

bool account_from_seckey(Account* account, const Seckey* seckey) {
    account->type = Account::SECKEY_IN_MEMORY;

    if (!get_public_key(seckey, &account->pubkey)) {
        printf("Failed to compute pubkey from seckey.\n");
        return false;
    }

    seckey_pad(seckey, &account->seckey_padded);
    return true;
}

void account_sign_event(const Account* account, const Event* event, AccountSignEventCallback cb) {
    if (account->type != Account::SECKEY_IN_MEMORY) {
        cb(true, "Can't sign events without a private key", NULL);
        return;
    }

    uint8_t event_copy_buf[Event::size_of(event)];
    memcpy(event_copy_buf, event, Event::size_of(event));
    Event* event_copy = (Event*)event_copy_buf;

    Seckey seckey;
    seckey_pad(&account->seckey_padded, &seckey);
    auto res = event_finish(event_copy, &seckey);
    memset(&seckey, 0, sizeof(Seckey));

    if (!res) {
        cb(true, "Failed to sign", NULL);
    } else {
        cb(false, NULL, event_copy);
    }
}

void account_nip04_encrypt(const Account* account, const Pubkey* pubkey, const char* plaintext,  uint32_t len, AccountNIP04EncryptCallback cb) {
    if (account->type != Account::SECKEY_IN_MEMORY) {
        cb(true, "Can't encrypt events without a private key", NULL, 0);
        return;
    }

    char ciphertext_out[32 + len * 2];
    uint32_t len_out;

    Seckey seckey;
    seckey_pad(&account->seckey_padded, &seckey);
    auto res = nip04_encrypt(pubkey, &seckey, plaintext, len, ciphertext_out, &len_out);
    memset(&seckey, 0, sizeof(Seckey));

    if (!res) {
        cb(true, "Failed to encrypt", NULL, 0);
    } else {
        cb(false, NULL, ciphertext_out, len_out);
    }
}

void account_nip04_decrypt(const Account* account, const Pubkey* pubkey, const char* ciphertext, uint32_t len, AccountNIP04DecryptCallback cb) {
    if (account->type != Account::SECKEY_IN_MEMORY) {
        cb(true, "Can't decrypt events without a private key", NULL, 0);
        return;
    }

    char plaintext_out[len];
    uint32_t len_out;

    Seckey seckey;
    seckey_pad(&account->seckey_padded, &seckey);
    auto res = nip04_decrypt(pubkey, &seckey, ciphertext, len, plaintext_out, &len_out);
    memset(&seckey, 0, sizeof(Seckey));

    if (!res) {
        cb(true, "Failed to decrypt", NULL, 0);
    } else {
        cb(false, NULL, plaintext_out, len_out);
    }
}
