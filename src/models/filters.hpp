//
//  filters.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-05-07.
//

#pragma once

#include "keys.hpp"
#include "relative.hpp"
#include "../utils/stackbuffer.hpp"
#include <string.h>

struct Filters {

    // Filter header (contains size)
    uint32_t __size;

    RelArray<EventId> ids;
    RelArray<Pubkey> authors;
    RelArray<uint32_t> kinds;
    RelArray<EventId> e_tags;
    RelArray<Pubkey> p_tags;
    int64_t since;
    int64_t until;
    int64_t limit;

    uint8_t __buffer[];

    static uint32_t size_of(const Filters* filters) {
        return filters->__size;
    }
};

struct FiltersBuilder {
    StackBuffer* buffer;
    uint32_t buffer_used;

    FiltersBuilder(StackBuffer* buffer_) {
        buffer = buffer_;
        buffer_used = sizeof(Filters);
        buffer->reserve(buffer_used);

        auto filt = filters();
        memset(filt, 0, sizeof(Filters));
        filt->since = -1;
        filt->until = -1;
        filt->limit = -1;
    }
    FiltersBuilder& ids(uint32_t count, const EventId* ids) {
        filters()->ids = copy_array(count, ids);
        return *this;
    }
    FiltersBuilder& authors(uint32_t count, const Pubkey* authors) {
        filters()->authors = copy_array(count, authors);
        return *this;
    }
    FiltersBuilder& kinds(uint32_t count, const uint32_t* kinds) {
        filters()->kinds = copy_array(count, kinds);
        return *this;
    }
    FiltersBuilder& e_tags(uint32_t count, const EventId* e_tags) {
        filters()->e_tags = copy_array(count, e_tags);
        return *this;
    }
    FiltersBuilder& p_tags(uint32_t count, const Pubkey* p_tags) {
        filters()->p_tags = copy_array(count, p_tags);
        return *this;
    }
    FiltersBuilder& id(const EventId* id) {
        filters()->ids = copy_array(1, id);
        return *this;
    }
    FiltersBuilder& author(const Pubkey* author) {
        filters()->authors = copy_array(1, author);
        return *this;
    }
    FiltersBuilder& kind(uint32_t kind) {
        filters()->kinds = copy_array(1, &kind);
        return *this;
    }
    FiltersBuilder& e_tag(const EventId* e_tag) {
        filters()->e_tags = copy_array(1, e_tag);
        return *this;
    }
    FiltersBuilder& p_tag(const Pubkey* p_tag) {
        filters()->p_tags = copy_array(1, p_tag);
        return *this;
    }
    FiltersBuilder& since(int64_t since) {
        filters()->since = since;
        return *this;
    }
    FiltersBuilder& until(int64_t until) {
        filters()->until = until;
        return *this;
    }
    FiltersBuilder& limit(int64_t limit) {
        filters()->limit = limit;
        return *this;
    }
    Filters* get() {
        auto filt = filters();
        filt->__size = buffer_used;
        return filt;
    }

private:
    template <typename T>
    RelArray<T> copy_array(uint32_t count, const T* data) {
        size_t size = sizeof(T) * count;
        buffer->reserve(buffer_used + size);

        memcpy((int8_t*)buffer->data + buffer_used, data, size);

        RelArray<T> rel_array;
        rel_array.size = count;
        rel_array.data.offset = buffer_used;

        buffer_used += size;

        return rel_array;
    }

    Filters* filters() {
        return (Filters*)buffer->data;
    }
};
