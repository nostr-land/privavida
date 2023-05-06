//
//  relay_message_stringify.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-10.
//

#include "relay_message.hpp"
#include "hex.hpp"
#include <rapidjson/writer.h>

void relay_message_stringify(const Event* event, char* output) {

    auto _base = (const void*)event;
    char buffer[128];

    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);

    writer.StartObject();

    writer.String("id");
    hex_encode(buffer, event->id.data, sizeof(event->id));
    writer.String(buffer, sizeof(event->id) * 2);

    writer.String("pubkey");
    hex_encode(buffer, event->pubkey.data, sizeof(event->pubkey));
    writer.String(buffer, sizeof(event->pubkey) * 2);

    writer.String("kind");
    writer.Uint(event->kind);

    writer.String("created_at");
    writer.Uint64(event->created_at);

    writer.String("content");
    writer.String(event->content.data.get(_base), event->content.size);

    writer.String("tags");
    writer.StartArray();
    for (int i = 0; i < event->tags.size; ++i) {
        writer.StartArray();
        auto tag = event->tags.get(_base, i).get(_base);
        for (int j = 0; j < tag.size; ++j) {
            writer.String(tag[j].data.get(_base), tag[j].size);
        }
        writer.EndArray();
    }
    writer.EndArray();

    writer.String("sig");
    hex_encode(buffer, event->sig.data, sizeof(event->sig));
    writer.String(buffer, sizeof(event->sig) * 2);

    writer.EndObject();

    strcpy(output, sb.GetString());
}
