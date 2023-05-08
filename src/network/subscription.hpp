//
//  subscription.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 07/05/2023.
//

#pragma once

#include "../models/event.hpp"
#include "../models/filters.hpp"
#include "../utils/stackbuffer.hpp"

struct Subscription {
    uint32_t __size;

    int32_t id;
    RelPointer<Filters> filters;
    RelArray<RelString> relays;

    uint8_t __buffer[];

    static uint32_t size_of(const Subscription* subscription) {
        return subscription->__size;
    }
};

struct SubscriptionBuilder {
    StackBuffer* buffer;
    uint32_t buffer_used;

    SubscriptionBuilder(StackBuffer* buffer_) {
        buffer = buffer_;
        buffer_used = sizeof(Filters);
        buffer->reserve(buffer_used);

        auto sub = subscription();
        memset(sub, 0, sizeof(Subscription));
    }
    SubscriptionBuilder& filters(const Filters* filters) {
        uint32_t size = Filters::size_of(filters);
        buffer->reserve(buffer_used + size);

        memcpy((int8_t*)buffer->data + buffer_used, filters, size);

        subscription()->filters.offset = buffer_used;

        buffer_used += size;

        return *this;
    }
    SubscriptionBuilder& relays(const Array<const char*>* relays) {

        // Reserve space for the array
        uint32_t size_array = relays->size * sizeof(RelString);
        buffer->reserve(buffer_used + size_array);

        subscription()->relays = RelArray<RelString>(relays->size, buffer_used);
        buffer_used += size_array;

        // Reserve space for the strings
        auto relays_out = subscription()->relays.get(subscription());
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
            auto rel_string = relays_out[i].get(subscription());
            strncpy(rel_string.data, (*relays)[i], rel_string.size);
        }

        return *this;
    }
    Subscription* get() {
        auto sub = subscription();
        sub->__size = buffer_used;
        return sub;
    }

private:
    Subscription* subscription() {
        return (Subscription*)buffer->data;
    }
};
