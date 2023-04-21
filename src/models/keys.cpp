//
//  keys.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-21.
//

#include "keys.hpp"

extern "C" {
#include <sha256/sha256.h>
#include <secp256k1_schnorrsig.h>
}

bool get_public_key(const Seckey* seckey, Pubkey* pubkey_out) {

    static auto secp256k1_context = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);

    secp256k1_pubkey pubkey;
    if (!secp256k1_ec_pubkey_create(secp256k1_context, &pubkey, seckey->data)) {
        return false;
    }

    secp256k1_xonly_pubkey xonly_pubkey;
    if (!secp256k1_xonly_pubkey_from_pubkey(secp256k1_context, &xonly_pubkey, NULL, &pubkey)) {
        return false;
    }

    secp256k1_xonly_pubkey_serialize(secp256k1_context, pubkey_out->data, &xonly_pubkey);
    return true;
}
