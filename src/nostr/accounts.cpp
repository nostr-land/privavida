//
//  accounts.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-21.
//

#include "accounts.hpp"
#include "../models/hex.hpp"
#include "../models/nip04.hpp"
#include "../ui.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct AccountWithSeckeyInMemory : public Account {
    Seckey seckey_padded;
};


int account_selected = -1;
Array<Account*> accounts = Array<Account*>(0, NULL);
AccountResponseCallback account_response_callback = NULL;

static bool write_default_account(const char* file_name);

static void seckey_pad(const Seckey* seckey_in, Seckey* seckey_out);
static Account* create_account(Account::Type account_type, const uint8_t* data, uint32_t length);

bool accounts_load() {

    static bool did_load = false;
    static bool load_success;
    if (did_load) {
        return load_success;
    }
    did_load = true;

    // For now, we're hard-coding a single account
    auto file_name = ui::get_user_data_path("account.bin");
    FILE* f = fopen(file_name, "rb");

    // If no account is set up, we start with default hardcoded account
    if (!f) {
        write_default_account(file_name);
        ui::user_data_flush();
        f = fopen(file_name, "rb");
    }
    if (!f) {
        printf("Failed to open '%s'\n", file_name);
        return (load_success = false);
    }

    // Get the entire file size
    fseek(f, 0, SEEK_END);
    auto len = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (!len) {
        printf("Couldn't read '%s', file is empty\n", file_name);
        fclose(f);
        return (load_success = false);
    }

    // Read the file
    uint8_t data[len];
    if (!fread(data, 1, len, f)) {
        printf("Couldn't read '%s'\n", file_name);
        fclose(f);
        return (load_success = false);
    }
    fclose(f);

    // First byte is the account type, the rest is the payload
    auto account = create_account((Account::Type)data[0], &data[1], len - 1);
    if (!account) {
        return (load_success = false);
    }

    accounts.size = 1;
    accounts.data = (Account**)malloc(accounts.size * sizeof(Account*));
    accounts[0] = account;
    account_selected = 0;
    return (load_success = true);
}

void account_sign_event(const Account* account, const Event* event, void* user_data) {

}

void account_nip04_encrypt(const Account* account, const Pubkey* pubkey, const char* plaintext,  uint32_t len, void* user_data) {

}

void account_nip04_decrypt(const Account* account, const Pubkey* pubkey, const char* ciphertext, uint32_t len, void* user_data) {
    if (account->type != Account::SECKEY_IN_MEMORY) {
        // Decrypt not supported!
        AccountResponse response;
        response.action = AccountResponse::NIP04_DECRYPT;
        response.user_data = user_data;
        response.error = true;
        response.error_reason = "Can't decrypt without a private key";
        account_response_callback(&response);
        return;
    }

    auto account_with_seckey = (const AccountWithSeckeyInMemory*)account;

    char plaintext_out[len];
    uint32_t len_out;

    Seckey seckey;
    seckey_pad(&account_with_seckey->seckey_padded, &seckey);
    auto res = nip04_decrypt(pubkey, &seckey, ciphertext, len, plaintext_out, &len_out);
    memset(&seckey, 0, sizeof(Seckey));

    AccountResponse response;
    response.action = AccountResponse::NIP04_DECRYPT;
    response.user_data = user_data;

    if (!res) {
        response.error = true;
        response.error_reason = "Failed to decrypt";
    } else {
        response.error = false;
        response.result_nip04 = plaintext_out;
        response.result_nip04_len = len_out;
    }

    account_response_callback(&response);
}


bool write_default_account(const char* file_name) {
    FILE* f = fopen(file_name, "wb");
    if (!f) {
        printf("Failed to create file: '%s'\n", file_name);
        return false;
    }

    const char* PUB_MEWJ = "489ac583fc30cfbee0095dd736ec46468faa8b187e311fda6269c4e18284ed0c";
    Account account;
    account.type = Account::PUBKEY_ONLY;
    hex_decode(account.pubkey.data, PUB_MEWJ, sizeof(Pubkey));

    if (!fwrite(&account.type, 1, 1, f)) {
        printf("Failed to write to file: '%s'\n", file_name);
        return false;
    }

    if (!fwrite(account.pubkey.data, 1, sizeof(Pubkey), f)) {
        printf("Failed to write to file: '%s'\n", file_name);
        return false;
    }

    fclose(f);
    return true;
}

void seckey_pad(const Seckey* seckey_in, Seckey* seckey_out) {
    auto ints_in  = (const uint64_t*)seckey_in->data;
    auto ints_out = (uint64_t*)seckey_out->data;

    // We've hardcoded a pad into the program. This is just to
    // prevent seckey being stored in plaintext on disk.
    // TODO: find a better method

    const uint8_t PAD[64] = {
        0x03, 0x2b, 0x8a, 0xcc, 0x87, 0x4d, 0x01, 0x85,
        0x01, 0x80, 0x3e, 0x37, 0x29, 0x9e, 0x40, 0x1b,
        0xfa, 0xb5, 0x88, 0xb6, 0x1e, 0x65, 0x5f, 0xcd,
        0x3f, 0xdc, 0x5e, 0x0e, 0x76, 0x4d, 0x18, 0xe0
    };
    for (int i = 0; i < sizeof(Seckey); ++i) {
        seckey_out->data[i] = PAD[i] ^ seckey_in->data[i];
    }
}

Account* create_account(Account::Type account_type, const uint8_t* data, uint32_t length) {

    switch (account_type) {
        case Account::PUBKEY_ONLY: {

            auto account = new Account;
            account->type = Account::PUBKEY_ONLY;

            // Pull pubkey from file
            if (length != sizeof(Pubkey)) {
                delete account;
                return NULL;
            }
            memcpy(account->pubkey.data, data, length);

            return account;
        }

        case Account::SECKEY_IN_MEMORY: {

            auto account = new AccountWithSeckeyInMemory;
            account->type = Account::SECKEY_IN_MEMORY;

            // Pull seckey_padded from file
            if (length != sizeof(Seckey)) {
                delete account;
                return NULL;
            }
            memcpy(account->seckey_padded.data, data, length);

            // Unpad to compute the pubkey
            Seckey seckey;
            seckey_pad(&account->seckey_padded, &seckey);
            auto res = get_public_key(&seckey, &account->pubkey);
            memset(&seckey, 0, sizeof(Seckey));

            // Failed to compute the pubkey
            if (!res) {
                printf("Failed to compute pubkey from file.\n");
                delete account;
                return NULL;
            }

            return account;
        }

        case Account::SECKEY_ON_SECURE_DEVICE: {

            printf("Seckey on secure device not supported yet.\n");
            return NULL;
        }
    }

    printf("Invalid account type.\n");
    return NULL;
}
