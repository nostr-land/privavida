//
//  relay.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 07/05/2023.
//

#pragma once

#include "../models/event.hpp"
#include "../models/filters.hpp"
#include "../utils/stackbuffer.hpp"

#define MAX_SUBS_PER_RELAY 10
#define MAX_QUEUED_SUBS_PER_RELAY 

struct RelaySubscription {

    enum EventOrdering {
        UNKNOWN_ORDER,
        NOT_ORDERED,
        ORDER_OLDER_TO_NEWER,
        ORDER_NEWER_TO_OLDER
    };

    int32_t subscription_id;
    bool keep_alive_after_eose;
    bool is_active;
    EventOrdering event_ordering;
    uint32_t time_subscribed;
    uint32_t oldest_event_created_at;
    uint32_t newest_event_created_at;
    int32_t num_events_received;

};

struct Relay {
    uint32_t __size;

    int32_t id;
    StackArrayFixed<char, 64> url;
    bool connected;
    int32_t socket_id;
    StackArrayFixed<RelaySubscription, MAX_SUBS_PER_RELAY> subscriptions;

    uint8_t __buffer[];

    static uint32_t size_of(const Relay* relay) {
        return relay->__size;
    }
};

struct RelayBuilder {
    StackBuffer* buffer;
    uint32_t buffer_used;

    SubscriptionBuilder(StackBuffer* buffer_) {
        buffer = buffer_;
        buffer_used = sizeof(Filters);
        buffer->reserve(buffer_used);

        auto sub = relay();
        memset(sub, 0, sizeof(Subscription));
    }
    SubscriptionBuilder& filters(const Filters* filters) {
        uint32_t size = Filters::size_of(filters);
        buffer->reserve(buffer_used + size);

        memcpy((int8_t*)buffer->data + buffer_used, filters, size);

        relay()->filters.offset = buffer_used;

        buffer_used += size;

        return *this;
    }
    SubscriptionBuilder& relays(const Array<const char*>* relays) {

        // Reserve space for the array
        uint32_t size_array = relays->size * sizeof(RelString);
        buffer->reserve(buffer_used + size_array);

        relay()->relays = RelArray<RelString>(relays->size, buffer_used);
        buffer_used += size_array;

        // Reserve space for the strings
        auto relays_out = relay()->relays.get(relay());
        uint32_t size_strings = 0;
        for (int i = 0; i < relays->size; ++i) {
            auto string = (*relays)[i];
            auto length = (uint32_t)strlen(string);

            relays_out[i] = RelString(length, buffer_used + size_strings);
            size_strings += length + 1;
        }
        buffer->reserve(buffer_used + size_strings);

        // Copy over the strings
        for (int i = 0; i < relays->size; ++i) {
            auto rel_string = relays_out[i].get(relay());
            strncpy(rel_string.data, (*relays)[i], rel_string.size);
        }

        return *this;
    }
    Subscription* get() {
        auto sub = relay();
        sub->__size = buffer_used;
        return sub;
    }

private:
    Subscription* relay() {
        return (Subscription*)buffer->data;
    }
};
