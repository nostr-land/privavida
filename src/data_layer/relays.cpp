//
//  relays.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-22.
//

#include "relays.hpp"
#include <vector>

namespace data_layer {

static std::vector<RelayInfo*> relays;

const RelayInfo* get_relay_info(int32_t relay_id) {
    for (auto relay : relays) {
        if (relay->id == relay_id) {
            return relay;
        }
    }

    return NULL;
}

const RelayInfo* get_relay_info(const char* relay_url) {
    auto relay_url_len = strlen(relay_url);

    for (auto relay : relays) {
        if (relay->url.size == relay_url_len &&
            strncmp(relay->url.data.get(relay), relay_url, relay_url_len) == 0) {
            return relay;
        }
    }

    static int32_t next_id = 1;
    StackBufferFixed<128> buffer;
    auto relay = RelayInfoBuilder(&buffer)
        .id(next_id++)
        .url(relay_url)
        .get();

    auto relay_copy = (RelayInfo*)malloc(RelayInfo::size_of(relay));
    memcpy(relay_copy, relay, RelayInfo::size_of(relay));

    relays.push_back(relay_copy);
    return relay_copy;
}

Array<int32_t> get_default_relays() {
    static int32_t relays[] = {
        get_relay_info("wss://relay.damus.io")->id,
        get_relay_info("wss://relay.snort.io")->id,
        get_relay_info("wss://eden.nostr.land")->id
    };
    return Array<int32_t>(3, relays);
}

}
