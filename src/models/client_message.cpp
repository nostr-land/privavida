//
//  client_message.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-10.
//

#include "client_message.hpp"
#include "event_stringify.hpp"
#include "hex.hpp"
#include <rapidjson/writer.h>

// This is struct that implements the rapidjson interface for
// an output stream. This allows us to stream the output from
// rapidjson into a StackBuffer
struct StackBufferWriter {
    typedef char Ch;

    StackBuffer& sb;
    uint32_t len;

    StackBufferWriter(StackBuffer* sb) : sb(*sb), len(0) {}
    void Put(char c) {
        sb.reserve(len + 1);
        ((char*)sb.data)[len++] = c;
    }
    void Flush() {
        Put('\0');
    }
};

static void write_filters(rapidjson::Writer<StackBufferWriter>& writer, const Filters* filters);

const char* client_message_req(const char* subscription_id, const Filters* filters, StackBuffer* stack_buffer) {

    StackBufferWriter sb(stack_buffer);
    rapidjson::Writer<StackBufferWriter> writer(sb);

    writer.StartArray();
    writer.String("REQ");
    writer.String(subscription_id);
    write_filters(writer, filters);
    writer.EndArray();

    return (const char*)stack_buffer->data;

}

const char* client_message_close(const char* subscription_id, StackBuffer* stack_buffer) {

    StackBufferWriter sb(stack_buffer);
    rapidjson::Writer<StackBufferWriter> writer(sb);

    writer.StartArray();
    writer.String("CLOSE");
    writer.String(subscription_id);
    writer.EndArray();

    return (const char*)stack_buffer->data;

}

const char* client_message_event(const Event* event, StackBuffer* stack_buffer) {
    return event_stringify(event, stack_buffer, true);
}


void write_filters(rapidjson::Writer<StackBufferWriter>& writer, const Filters* filters) {

    char hex_buffer[65];
    hex_buffer[64] = '\0';

    writer.StartObject();
    
    // ids
    if (filters->ids.size) {
        writer.String("ids");
        writer.StartArray();
        for (auto& id : filters->ids.get(filters)) {
            hex_encode(hex_buffer, id.data, sizeof(EventId));
            writer.String(hex_buffer);
        }
        writer.EndArray();
    }

    // authors
    if (filters->authors.size) {
        writer.String("authors");
        writer.StartArray();
        for (auto& pubkey : filters->authors.get(filters)) {
            hex_encode(hex_buffer, pubkey.data, sizeof(Pubkey));
            writer.String(hex_buffer);
        }
        writer.EndArray();
    }

    // kinds
    if (filters->kinds.size) {
        writer.String("kinds");
        writer.StartArray();
        for (auto kind : filters->kinds.get(filters)) {
            writer.Int(kind);
        }
        writer.EndArray();
    }

    // e_tags
    if (filters->e_tags.size) {
        writer.String("#e");
        writer.StartArray();
        for (auto& id : filters->e_tags.get(filters)) {
            hex_encode(hex_buffer, id.data, sizeof(EventId));
            writer.String(hex_buffer);
        }
        writer.EndArray();
    }

    // p_tags
    if (filters->p_tags.size) {
        writer.String("#p");
        writer.StartArray();
        for (auto& pubkey : filters->p_tags.get(filters)) {
            hex_encode(hex_buffer, pubkey.data, sizeof(Pubkey));
            writer.String(hex_buffer);
        }
        writer.EndArray();
    }

    // since
    if (filters->since != -1) {
        writer.String("since");
        writer.Int64(filters->since);
    }

    // until
    if (filters->until != -1) {
        writer.String("until");
        writer.Int64(filters->until);
    }

    // limit
    if (filters->limit != -1) {
        writer.String("limit");
        writer.Int64(filters->limit);
    }

    writer.EndObject();
}
