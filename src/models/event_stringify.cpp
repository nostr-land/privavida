//
//  event_stringify.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-10.
//

#include "event_stringify.hpp"
#include "hex.hpp"
#include <rapidjson/writer.h>

// This is struct that implements the rapidjson interface for
// an output stream. This allows us to stream the output from
// rapidjson into a StackBuffer
// @TODO: dedupe this as it exists both in client_message.cpp & here
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

const char* event_stringify(const Event* event, StackBuffer* stack_buffer, bool wrap_in_event_message) {

    StackBufferWriter sb(stack_buffer);
    rapidjson::Writer<StackBufferWriter> writer(sb);

    char hex_buffer[128];

    if (wrap_in_event_message) {
        writer.StartArray();
        writer.String("EVENT");
    }

    writer.StartObject();

    writer.String("id");
    hex_encode(hex_buffer, event->id.data, sizeof(event->id));
    writer.String(hex_buffer, sizeof(event->id) * 2);

    writer.String("pubkey");
    hex_encode(hex_buffer, event->pubkey.data, sizeof(event->pubkey));
    writer.String(hex_buffer, sizeof(event->pubkey) * 2);

    writer.String("kind");
    writer.Uint(event->kind);

    writer.String("created_at");
    writer.Uint64(event->created_at);

    writer.String("content");
    writer.String(event->content.data.get(event), event->content.size);

    writer.String("tags");
    writer.StartArray();
    for (int i = 0; i < event->tags.size; ++i) {
        writer.StartArray();
        auto tag = event->tags.get(event, i).get(event);
        for (int j = 0; j < tag.size; ++j) {
            writer.String(tag[j].data.get(event), tag[j].size);
        }
        writer.EndArray();
    }
    writer.EndArray();

    writer.String("sig");
    hex_encode(hex_buffer, event->sig.data, sizeof(event->sig));
    writer.String(hex_buffer, sizeof(event->sig) * 2);

    writer.EndObject();

    if (wrap_in_event_message) {
        writer.EndArray();
    }

    return (const char*)stack_buffer->data;
}
