//
//  accounts.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-21.
//

#include "accounts.hpp"
#include "../models/hex.hpp"
#include <app.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace data_layer {

int account_selected = -1;
std::vector<Account> accounts;

static bool write_default_account(const char* file_name) {
    FILE* f = fopen(file_name, "wb");
    if (!f) {
        printf("Failed to create file: '%s'\n", file_name);
        return false;
    }

    const char* DEFAULT_DATA = "00489ac583fc30cfbee0095dd736ec46468faa8b187e311fda6269c4e18284ed0c";
    uint8_t default_data[33];
    hex_decode(default_data, DEFAULT_DATA, sizeof(default_data));
    if (!fwrite(default_data, 1, sizeof(default_data), f)) {
        printf("Failed to write to file: '%s'\n", file_name);
        return false;
    }

    fclose(f);
    return true;
}

bool accounts_load() {

    static bool did_load = false;
    static bool load_success;
    if (did_load) {
        return load_success;
    }
    did_load = true;

    // For now, we're hard-coding a single account
    auto file_name = app::get_user_data_path("account.bin");
    FILE* f = fopen(file_name, "rb");

    // If no account is set up, we start with default hardcoded account
    if (!f) {
        write_default_account(file_name);
        app::user_data_flush();
        f = fopen(file_name, "rb");
    }
    if (!f) {
        printf("Failed to open '%s'\n", file_name);
        return (load_success = false);
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
    return (load_success = true);
}

}
