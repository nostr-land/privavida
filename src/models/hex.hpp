//
//  hex.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 18/07/2018.
//

#pragma once
#include <inttypes.h>

extern const char hex_lookup[16];
extern const int8_t hex_lookup_rev[128];

bool hex_decode(uint8_t* output, const char* input, int output_len);
void hex_encode(char* output, const uint8_t* input, int input_len);
