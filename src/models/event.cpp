//
//  event.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-10.
//

#include "event.hpp"

#include "hex.hpp"
#include "time.h"
#include <rapidjson/writer.h>

extern "C" {
#include <sha256/sha256.h>
#include <secp256k1_schnorrsig.h>
}

static_assert(sizeof(EventId) == 32);

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

void event_compute_hash(const Event* event, EventId* hash_out) {

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
            char hex[sizeof(Pubkey) * 2];
            hex_encode(hex, event->pubkey.data, sizeof(Pubkey));
            writer.String(hex, sizeof(Pubkey) * 2);
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
                writer.String(tag[j].data.get(_base), tag[j].size);
            }
            writer.EndArray();
        }
        writer.EndArray();

        // <content, as a string>
        writer.String(event->content.data.get(_base), event->content.size);

    }
    writer.EndArray();

    sha256_final(&ctx, hash_out->data);
}

bool event_validate(Event* event) {

    static auto secp256k1_context = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);

    // Check the hash
    EventId hash_computed;
    event_compute_hash(event, &hash_computed);
    if (!compare_keys(&event->id, &hash_computed)) {
        event->validity = EVENT_INVALID_ID;
        return false;
    }

    // Verify signature
    secp256k1_xonly_pubkey pubkey;
    if (!secp256k1_xonly_pubkey_parse(secp256k1_context, &pubkey, event->pubkey.data) ||
        !secp256k1_schnorrsig_verify(secp256k1_context, event->sig.data, event->id.data, 32, &pubkey)) {
        event->validity = EVENT_INVALID_SIG;
        return false;
    }

    event->validity = EVENT_VALID;
    return true;
}

bool event_finish(Event* event, const Seckey* seckey) {

    static auto secp256k1_context = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
    // TODO: call sec256k1_content_randomize() with a random seed!!

    // event->pubkey
    if (!get_public_key(seckey, &event->pubkey)) return false;
    event->created_at = time(NULL);
    event_compute_hash(event, &event->id);

    // event->sig
    {
        secp256k1_keypair keypair;
        if (!secp256k1_keypair_create(secp256k1_context, &keypair, seckey->data)) {
            return false;
        }

        if (!secp256k1_schnorrsig_sign32(secp256k1_context, event->sig.data, event->id.data, &keypair, NULL)) {
            return false;
        }
    }

    return event_validate(event);
}
