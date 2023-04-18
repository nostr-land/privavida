//
//  event.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 18/07/2018.
//

#include "event.hpp"

#include "hex.hpp"
#include "time.h"
#include <rapidjson/writer.h>

extern "C" {
#include <sha256/sha256.h>
#include <secp256k1_schnorrsig.h>
}

static_assert(sizeof(Event::id) == 32);

// This is struct that implements the rapidjson interface for
// an output stream. This allows us to stream the output from
// the rapidjson JSON writer directly into the sha256 algo.
struct Sha256Writer {
    typedef char Ch;

    SHA256_CTX* ctx;
    void Put(char c) {
        sha256_update(ctx, (uint8_t*)&c, 1);
    }
    void Flush() {}
};

void event_compute_hash(const Event* event, uint8_t* hash_out) {

    auto _base = (void*)event;

    SHA256_CTX ctx;
    sha256_init(&ctx);

    Sha256Writer sha256_writer;
    sha256_writer.ctx = &ctx;
    rapidjson::Writer<Sha256Writer> writer(sha256_writer);

    // According to the NIP-01 spec:
    // [
    //   0,
    //   <pubkey, as a (lowercase) hex string>,
    //   <created_at, as a number>,
    //   <kind, as a number>,
    //   <tags, as an array of arrays of non-null strings>,
    //   <content, as a string>
    // ]

    writer.StartArray();
    {

        // 0,
        writer.Uint(0);

        // <pubkey, as a (lowercase) hex string>,
        {
            char hex[sizeof(Event::pubkey) * 2];
            hex_encode(hex, event->pubkey, sizeof(Event::pubkey));
            writer.String(hex, sizeof(Event::pubkey) * 2);
        }

        // <created_at, as a number>,
        writer.Uint64(event->created_at);

        // <kind, as a number>,
        writer.Uint(event->kind);

        // <tags, as an array of arrays of non-null strings>,
        writer.StartArray();
        for (int i = 0; i < event->tags.size; ++i) {
            writer.StartArray();
            auto tag = event->tags.get(_base, i).get(_base);
            for (int j = 0; j < tag.size; ++j) {
                if (tag[j].size) {
                    writer.String(tag[j].data.get(_base), tag[j].size);
                }
            }
            writer.EndArray();
        }
        writer.EndArray();

        // <content, as a string>
        writer.String(event->content.data.get(_base), event->content.size);

    }
    writer.EndArray();

    sha256_final(&ctx, hash_out);
}

bool event_validate(Event* event) {

    static auto secp256k1_context = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);

    // Check the hash
    uint8_t hash[32];
    event_compute_hash(event, hash);

    auto hash_event_id = (const uint64_t*)event->id;
    auto hash_computed = (const uint64_t*)hash;
    if (hash_event_id[0] != hash_computed[0] ||
        hash_event_id[1] != hash_computed[1] ||
        hash_event_id[2] != hash_computed[2] ||
        hash_event_id[3] != hash_computed[3]) {
        event->validity = EVENT_INVALID_ID;
        return false;
    }

    // Verify signature
    secp256k1_xonly_pubkey pubkey;
    if (!secp256k1_xonly_pubkey_parse(secp256k1_context, &pubkey, event->pubkey) ||
        !secp256k1_schnorrsig_verify(secp256k1_context, event->sig, event->id, 32, &pubkey)) {
        event->validity = EVENT_INVALID_SIG;
        return false;
    }

    event->validity = EVENT_VALID;
    return true;
}

bool event_finish(Event* event, const uint8_t* seckey) {

    static auto secp256k1_context = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
    // TODO: call sec256k1_content_randomize() with a random seed!!


    // event->pubkey
    {
        secp256k1_pubkey pubkey;
        if (!secp256k1_ec_pubkey_create(secp256k1_context, &pubkey, seckey)) {
            return false;
        }

        secp256k1_xonly_pubkey xonly_pubkey;
        if (!secp256k1_xonly_pubkey_from_pubkey(secp256k1_context, &xonly_pubkey, NULL, &pubkey)) {
            return false;
        }

        secp256k1_xonly_pubkey_serialize(secp256k1_context, event->pubkey, &xonly_pubkey);
    }

    // event->created_at
    {
        event->created_at = time(NULL);
    }

    // event->id
    {
        event_compute_hash(event, event->id);
    }

    // event->sig
    {
        secp256k1_keypair keypair;
        if (!secp256k1_keypair_create(secp256k1_context, &keypair, seckey)) {
            return false;
        }

        if (!secp256k1_schnorrsig_sign32(secp256k1_context, event->sig, event->id, &keypair, NULL)) {
            return false;
        }
    }

    return event_validate(event);
}
