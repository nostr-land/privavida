//
//  relay_info.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-05-08.
//

#pragma once

#include "keys.hpp"
#include "relative.hpp"
#include "../utils/stackbuffer.hpp"
#include <string.h>

typedef int16_t RelayId;

struct RelayInfo {

    // Relay info header (contains size)
    uint32_t __size;

    RelayId id;
    RelString url;

    uint8_t __buffer[];

    static uint32_t size_of(const RelayInfo* filters) {
        return filters->__size;
    }
};

struct RelayInfoBuilder {
    StackBuffer* buffer;
    uint32_t buffer_used;

    RelayInfoBuilder(StackBuffer* buffer_) {
        buffer = buffer_;
        buffer_used = sizeof(RelayInfo);
        buffer->reserve(buffer_used);

        memset(get(), 0, sizeof(RelayInfo));
    }
    RelayInfoBuilder& id(RelayId id) {
        get()->id = id;
        return *this;
    }
    RelayInfoBuilder& url(const char* url) {
        uint32_t len = (uint32_t)strlen(url);
        uint32_t size = len + 1;

        buffer->reserve(buffer_used + size);

        memcpy((int8_t*)buffer->data + buffer_used, url, size);

        get()->url = RelString(len, buffer_used);
        buffer_used += size;

        return *this;
    }
    RelayInfo* finish() {
        get()->__size = buffer_used;
        return get();
    }

private:
    RelayInfo* get() {
        return (RelayInfo*)buffer->data;
    }
};
