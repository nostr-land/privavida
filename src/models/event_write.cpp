//
//  event_write.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 18/07/2018.
//

#include "event_write.hpp"
#include "hex.hpp"
#include <rapidjson/writer.h>

void event_write(const Event* event, char* output) {

    auto _base = (const void*)event;
    char buffer[128];

    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);

    writer.StartObject();

    writer.String("id");
    hex_encode(buffer, event->id, sizeof(event->id));
    writer.String(buffer, sizeof(event->id) * 2);

    writer.String("pubkey");
    hex_encode(buffer, event->pubkey, sizeof(event->pubkey));
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
    hex_encode(buffer, event->sig, sizeof(event->sig));
    writer.String(buffer, sizeof(event->sig) * 2);

    writer.EndObject();

    strcpy(output, sb.GetString());
}
