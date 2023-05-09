//
//  accounts.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-21.
//

#include "accounts.hpp"
#include "relays.hpp"
#include "profiles.hpp"
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

static void open_default_subscriptions() {
    network::stop_all_tasks();
    
    auto account = current_account();
    if (!account) return;
    
    StackBufferFixed<128> filters_buffer;

    // "dms_sent" subscription
    {
        auto filters = FiltersBuilder(&filters_buffer)
            .kind(4)
            .author(&account->pubkey)
            .finish();

        for (auto relay_id : get_default_relays()) {
            network::relay_add_task_request(relay_id, filters);
            network::relay_add_task_stream(relay_id, filters);
        }
    }

    // "dms_received" subscription
    {
        auto filters = FiltersBuilder(&filters_buffer)
            .kind(4)
            .p_tag(&account->pubkey)
            .finish();

        for (auto relay_id : get_default_relays()) {
            network::relay_add_task_request(relay_id, filters);
            network::relay_add_task_stream(relay_id, filters);
        }
    }

    // "profile" subscription (fetches kind 0 metadata and kind 3 contact list)
    {
        uint32_t kinds[2] = { 0, 3 };
        auto filters = FiltersBuilder(&filters_buffer)
            .kinds(2, kinds)
            .author(&account->pubkey)
            .finish();

        for (auto relay_id : get_default_relays()) {
            network::relay_add_task_request(relay_id, filters);
            network::relay_add_task_stream(relay_id, filters);
        }
    }

    data_layer::batch_profile_requests();
}

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
    open_default_subscriptions();
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
    open_default_subscriptions();
    return true;
}

bool open_account_with_seckey(const Seckey* seckey) {
    Account account;
    if (!account_from_seckey(&account, seckey)) return false;
    if (!write_account(&account)) return false;
    accounts.clear();
    accounts.push_back(std::move(account));
    account_selected = 0;
    open_default_subscriptions();
    return true;
}

}
