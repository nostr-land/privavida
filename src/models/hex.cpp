//
//  hex.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-10.
//

#include "hex.hpp"

const char hex_lookup[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};

const int8_t hex_lookup_rev[128] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
    -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

bool hex_decode(uint8_t* output, const char* input, int output_len) {
    const char* in = input;
    const char* in_end = input + 2*output_len;
    uint8_t* out = output;
    while (in < in_end) {
        uint8_t ch_upper = *in++;
        uint8_t ch_lower = *in++;
        if (ch_upper >= 128 || ch_lower >= 128) return false;

        int hex_upper = hex_lookup_rev[ch_upper];
        int hex_lower = hex_lookup_rev[ch_lower];
        if (hex_upper == -1 || hex_lower == -1) return false;

        *out++ = (hex_upper << 4) + hex_lower;
    }

    return true;
}

void hex_encode(char* output, const uint8_t* input, int input_len) {
    for (int i = 0; i < input_len; ++i) {
        *output++ = hex_lookup[input[i] / 16];
        *output++ = hex_lookup[input[i] % 16];
    }
}
