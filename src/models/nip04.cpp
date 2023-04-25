//
//  nip04.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-21.
//

#include "nip04.hpp"
#include "hex.hpp"

#include <string.h>
#include <stdio.h>
#include <random>

extern "C" {
#include <sha256/sha256.h>
#include <secp256k1_schnorrsig.h>
#include <secp256k1_ecdh.h>
#include "c/aes.h"
#include "c/base64.h"
}

static bool decode_content(const char* ciphertext, uint32_t len, uint8_t* payload, uint32_t* payload_len, uint8_t* iv);
static bool is_valid_unicode(uint8_t* data, uint32_t* len);

static int ecdh_copy_xonly(uint8_t* output, const uint8_t* x32, const uint8_t* y32, void* data) {
    memcpy(output, x32, 32);
    return 1;
}

static bool compute_shared_secret(const Pubkey* pubkey, const Seckey* seckey, uint8_t* shared_secret) {

    static auto secp256k1_context = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);

    // Create pubkey with fixed y coordinate of 2
    uint8_t pubkey_with_y[33];
    pubkey_with_y[0] = 0x02;
    memcpy(&pubkey_with_y[1], pubkey->data, 32);

    // Parse pubkey
    secp256k1_pubkey secp_pubkey;
    if (!secp256k1_ec_pubkey_parse(secp256k1_context, &secp_pubkey, pubkey_with_y, sizeof(pubkey_with_y))) {
        return false;
    }

    // Compute shared secret
    if (!secp256k1_ecdh(secp256k1_context, shared_secret, &secp_pubkey, seckey->data, &ecdh_copy_xonly, NULL)) {
        return false;
    }

    return true;
}

bool nip04_decrypt(const Pubkey* pubkey, const Seckey* seckey, const char* ciphertext, uint32_t len, char* plaintext_out, uint32_t* len_out) {

    static auto secp256k1_context = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);

    // Step 1. Decode the content
    uint8_t payload[len], iv[AES_BLOCKLEN];
    uint32_t payload_len;
    if (!decode_content(ciphertext, len, payload, &payload_len, iv)) {
        return false;
    }

    // Step 2. Compute our shared secret
    uint8_t shared_secret[32];
    if (!compute_shared_secret(pubkey, seckey, shared_secret)) {
        return false;
    }

    // Step 3. Decrypt
    AES_ctx ctx;
    AES_init_ctx_iv(&ctx, shared_secret, iv);
    AES_CBC_decrypt_buffer(&ctx, payload, payload_len);

    // Step 4. Check & copy result
    payload_len = strlen((char*)payload);
    if (!is_valid_unicode(payload, &payload_len)) {
        return false;
    }
    memcpy(plaintext_out, payload, payload_len);
    *len_out = payload_len;
    return true;
}

bool nip04_encrypt(const Pubkey* pubkey, const Seckey* seckey, const char* plaintext, uint32_t len, char* ciphertext_out, uint32_t* len_out) {

    static auto secp256k1_context = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);

    if (len == 0) {
        return false;
    }

    // Step 1. Compute our shared secret
    uint8_t shared_secret[32];
    if (!compute_shared_secret(pubkey, seckey, shared_secret)) {
        return false;
    }

    // Step 2. Generate our initialization vector
    uint8_t iv[AES_BLOCKLEN] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    {
        static_assert(AES_BLOCKLEN == 16);
        static std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint32_t> distrib;
        ((uint32_t*)iv)[0] = distrib(gen);
        ((uint32_t*)iv)[1] = distrib(gen);
        ((uint32_t*)iv)[2] = distrib(gen);
        ((uint32_t*)iv)[3] = distrib(gen);
    }

    // Step 3. Prepare our payload
    uint32_t num_padding = 16 - len % 16;
    uint32_t payload_len = len + num_padding;
    uint8_t payload[payload_len];
    strncpy((char*)payload, plaintext, len);
    for (int i = len; i < payload_len; i++) {
        payload[i] = (uint8_t)num_padding;
    }

    // Step 4. Encrypt
    AES_ctx ctx;
    AES_init_ctx_iv(&ctx, shared_secret, iv);
    AES_CBC_encrypt_buffer(&ctx, payload, payload_len);

    // Step 5. Encode the result
    char* ch = ciphertext_out;
    ch += Base64encode(ch, payload, payload_len) - 1;
    strncpy(ch, "?iv=", 4);
    ch += 4;
    ch += Base64encode(ch, iv, sizeof(iv)) - 1;
    *len_out = (uint32_t)(ch - ciphertext_out);
    return true;
}

bool decode_content(const char* ciphertext, uint32_t len, uint8_t* payload, uint32_t* payload_len, uint8_t* iv) {

    // Find "?iv=" string
    int buffer_end_idx = -1;
    int iv_start_idx = -1;
    for (int i = 0; i + 4 < len; ++i) {
        if (strncmp(&ciphertext[i], "?iv=", 4) == 0) {
            buffer_end_idx = i;
            iv_start_idx = i + 4;
            break;
        }
    }
    if (buffer_end_idx == -1) return false;

    // Make a copy of the ciphertext so we can insert NULL-terminals
    char temp[len + 1];
    memcpy(temp, ciphertext, len);
    temp[buffer_end_idx] = '\0';
    temp[len] = '\0';

    // Decode payload
    *payload_len = Base64decode(payload, temp);

    // Decode iv (into temp first, to be safe)
    auto iv_len = Base64decode((uint8_t*)temp, &ciphertext[iv_start_idx]);
    if (iv_len != AES_BLOCKLEN) return false;

    // Copy iv to output
    memcpy(iv, (uint8_t*)temp, iv_len);

    return true;
}

static uint8_t valid_ascii[128] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x00 - 0x0F
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x10 - 0x1F
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x20 - 0x2F
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x30 - 0x3F
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x40 - 0x4F
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x50 - 0x5F
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x60 - 0x6F
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, // 0x70 - 0x7F
};

bool is_valid_unicode(uint8_t* data, uint32_t* len) {

    // Reference: https://en.wikipedia.org/wiki/UTF-8
    //
    // First code point   Last code point   Byte 1      Byte 2      Byte 3      Byte 4
    // U+0000             U+007F            0xxxxxxx    
    // U+0080             U+07FF            110xxxxx    10xxxxxx    
    // U+0800             U+FFFF            1110xxxx    10xxxxxx    10xxxxxx    
    // U+10000            U+10FFFF          11110xxx    10xxxxxx    10xxxxxx    10xxxxxx

    int i = 0;
    while (i < *len) {
        auto ch = data[i];
        auto ch_len = (
            !(ch & 0x80) ? 1 :
            !(ch & 0x20) ? 2 :
            !(ch & 0x10) ? 3 : 4
        );
        if (i + ch_len > *len) return false;

        // Handle unicode codepoints
        if (ch_len >= 2) {
            if (/*ch_len >= 2*/(data[i+1] & 0xC0) != 0x80) return false;
            if (ch_len >= 3 && (data[i+2] & 0xC0) != 0x80) return false;
            if (ch_len == 4 && (data[i+3] & 0xC0) != 0x80) return false;
            i += ch_len;
            continue;
        }

        // Handle ASCII characters
        if (ch <= 0x10) {
            // We've reached a PCKS padding byte, which signals the end of the message
            *len = i;
            return true;
        } else if (!valid_ascii[ch]) {
            return false;
        }
        i += 1;
    }

    return true;
}
