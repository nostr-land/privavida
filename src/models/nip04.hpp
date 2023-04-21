//
//  nip04.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-21.
//

#pragma once
#include "keys.hpp"

bool nip04_decrypt(const Pubkey* pubkey, const Seckey* seckey, const char* ciphertext, uint32_t len, char* plaintext_out, uint32_t* len_out);
bool nip04_encrypt(const Pubkey* pubkey, const Seckey* seckey, const char* plaintext, uint32_t len, char* ciphertext_out, uint32_t* len_out);
