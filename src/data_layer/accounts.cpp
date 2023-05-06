//
//  accounts.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-21.
//

#include "accounts.hpp"
#include "../models/hex.hpp"
#include "../network/network.hpp"
#include <app.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace data_layer {

constexpr auto ACCOUNT_FILE = "account0.bin";
static int account_selected = -1;
static std::vector<Account> accounts;

static bool write_account(const Account* account) {
    auto file_name = app::get_user_data_path(ACCOUNT_FILE);
    FILE* f = fopen(file_name, "wb");
    if (!f) {
        printf("Failed to create file: '%s'\n", file_name);
        return false;
    }

    bool success = account_store_to_file(account, f);

    fclose(f);
    app::user_data_flush();
    return success;
}

bool accounts_load() {

    static bool did_load = false;
    static bool load_success;
    if (did_load) {
        return load_success;
    }
    did_load = true;

    // For now, we're hard-coding a single account
    auto file_name = app::get_user_data_path(ACCOUNT_FILE);
    FILE* f = fopen(file_name, "rb");

    // No account set up?
    if (!f) {
        return (load_success = true);
    }

    // Read the entire file
    fseek(f, 0, SEEK_END);
    auto len = (int32_t)ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t data[len];
    if (!fread(data, 1, len, f)) {
        printf("Couldn't read '%s'\n", file_name);
        fclose(f);
        return (load_success = false);
    }
    fclose(f);

    // Read the account
    Account account;
    if (!account_load_from_file(&account, data, len)) {
        printf("Couldn't read account from file '%s'\n", file_name);
        return (load_success = false);
    }

    accounts.push_back(account);
    account_selected = 0;
    network::init();
    return (load_success = true);
}

const Account* current_account() {
    if (accounts.empty() || account_selected == -1) {
        return NULL;
    }
    return &accounts[account_selected];
}

bool open_account_with_pubkey(const Pubkey* pubkey) {
    Account account;
    if (!account_from_pubkey(&account, pubkey)) return false;
    if (!write_account(&account)) return false;
    accounts.clear();
    accounts.push_back(std::move(account));
    account_selected = 0;
    network::init();
    return true;
}

bool open_account_with_seckey(const Seckey* seckey) {
    Account account;
    if (!account_from_seckey(&account, seckey)) return false;
    if (!write_account(&account)) return false;
    accounts.clear();
    accounts.push_back(std::move(account));
    account_selected = 0;
    network::init();
    return true;
}

}
