/*
 * Centaurean Density
 *
 * Copyright (c) 2015, Guillaume Voirin
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Centaurean nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * 5/02/15 20:57
 *
 * --------------
 * Lion algorithm
 * --------------
 *
 * Author(s)
 * Guillaume Voirin (https://github.com/gpnuma)
 *
 * Description
 * Multiform compression algorithm
 */

#ifndef DENSITY_LION_H
#define DENSITY_LION_H

#include <stdint.h>
#include "kernel.h"

typedef enum {
    DENSITY_LION_FORM_CHUNK_PREDICTIONS,
    DENSITY_LION_FORM_CHUNK_DICTIONARY_A,
    DENSITY_LION_FORM_CHUNK_DICTIONARY_B,
    DENSITY_LION_FORM_SECONDARY_ACCESS,
    DENSITY_LION_NUMBER_OF_FORMS,
} DENSITY_LION_FORM;

typedef enum {
    DENSITY_LION_SIGNATURE_FLAG_BIGRAM_DICTIONARY = 0x0,
    DENSITY_LION_SIGNATURE_FLAG_BIGRAM_SECONDARY = 0x1,
    DENSITY_LION_SIGNATURE_FLAG_BIGRAM_ENCODED = 0x0,
    DENSITY_LION_SIGNATURE_FLAG_BIGRAM_PLAIN = 0x1,
} DENSITY_LION_SIGNATURE_FLAG;

#define DENSITY_LION_PREFERRED_BLOCK_SIGNATURES_SHIFT                   12
#define DENSITY_LION_PREFERRED_BLOCK_SIGNATURES                         (1 << DENSITY_LION_PREFERRED_BLOCK_SIGNATURES_SHIFT)

#define DENSITY_LION_PREFERRED_EFFICIENCY_CHECK_SIGNATURES_SHIFT        8
#define DENSITY_LION_PREFERRED_EFFICIENCY_CHECK_SIGNATURES              (1 << DENSITY_LION_PREFERRED_EFFICIENCY_CHECK_SIGNATURES_SHIFT)

#define DENSITY_LION_HASH32_MULTIPLIER                                  (uint32_t)0x9D6EF916lu
#define DENSITY_LION_CHUNK_HASH_BITS                                    16
#define DENSITY_LION_BIGRAM_HASH_BITS                                   8

#define DENSITY_LION_HASH_ALGORITHM(hash32, value32)                    hash32 = value32 * DENSITY_LION_HASH32_MULTIPLIER;\
                                                                        hash32 = (hash32 >> (32 - DENSITY_LION_CHUNK_HASH_BITS));

#define FORMAT(v)               0##v##llu

#define ISOLATE(b, p)           ((FORMAT(b) / p) & 0x1)

#define BINARY_TO_UINT(b)        ((ISOLATE(b, 1llu) ? 0x1 : 0)\
                                + (ISOLATE(b, 8llu) ? 0x2 : 0)\
                                + (ISOLATE(b, 64llu) ? 0x4 : 0)\
                                + (ISOLATE(b, 512llu) ? 0x8 : 0)\
                                + (ISOLATE(b, 4096llu) ? 0x10 : 0)\
                                + (ISOLATE(b, 32768llu) ? 0x20 : 0)\
                                + (ISOLATE(b, 262144llu) ? 0x40 : 0)\
                                + (ISOLATE(b, 2097152llu) ? 0x80 : 0)\
                                + (ISOLATE(b, 16777216llu) ? 0x100 : 0)\
                                + (ISOLATE(b, 134217728llu) ? 0x200 : 0)\
                                + (ISOLATE(b, 1073741824llu) ? 0x400 : 0)\
                                + (ISOLATE(b, 8589934592llu) ? 0x800 : 0)\
                                + (ISOLATE(b, 68719476736llu) ? 0x1000 : 0)\
                                + (ISOLATE(b, 549755813888llu) ? 0x2000 : 0)\
                                + (ISOLATE(b, 4398046511104llu) ? 0x4000 : 0)\
                                + (ISOLATE(b, 35184372088832llu) ? 0x8000 : 0)\
                                + (ISOLATE(b, 281474976710656llu) ? 0x10000 : 0)\
                                + (ISOLATE(b, 2251799813685248llu) ? 0x20000 : 0))

// Unary codes except the last one
#define DENSITY_LION_FORM_ENTROPY_CODES {\
    {BINARY_TO_UINT(0), 1},\
    {BINARY_TO_UINT(10), 2},\
    {BINARY_TO_UINT(110), 3},\
    {BINARY_TO_UINT(111), 3},\
}

// Golomb-Rice codes, k = 3, 32 values
#define DENSITY_LION_INDEX_ENTROPY_CODES {\
    {BINARY_TO_UINT(0000), 4},\
    {BINARY_TO_UINT(0001), 4},\
    {BINARY_TO_UINT(0010), 4},\
    {BINARY_TO_UINT(0011), 4},\
    {BINARY_TO_UINT(0100), 4},\
    {BINARY_TO_UINT(0101), 4},\
    {BINARY_TO_UINT(0110), 4},\
    {BINARY_TO_UINT(0111), 4},\
    {BINARY_TO_UINT(10000), 5},\
    {BINARY_TO_UINT(10001), 5},\
    {BINARY_TO_UINT(10010), 5},\
    {BINARY_TO_UINT(10011), 5},\
    {BINARY_TO_UINT(10100), 5},\
    {BINARY_TO_UINT(10101), 5},\
    {BINARY_TO_UINT(10110), 5},\
    {BINARY_TO_UINT(10111), 5},\
    {BINARY_TO_UINT(110000), 6},\
    {BINARY_TO_UINT(110001), 6},\
    {BINARY_TO_UINT(110010), 6},\
    {BINARY_TO_UINT(110011), 6},\
    {BINARY_TO_UINT(110100), 6},\
    {BINARY_TO_UINT(110101), 6},\
    {BINARY_TO_UINT(110110), 6},\
    {BINARY_TO_UINT(110111), 6},\
    {BINARY_TO_UINT(1110000), 7},\
    {BINARY_TO_UINT(1110001), 7},\
    {BINARY_TO_UINT(1110010), 7},\
    {BINARY_TO_UINT(1110011), 7},\
    {BINARY_TO_UINT(1110100), 7},\
    {BINARY_TO_UINT(1110101), 7},\
    {BINARY_TO_UINT(1110110), 7},\
    {BINARY_TO_UINT(1110111), 7},\
}

typedef struct {
    uint_fast32_t value;
    uint_fast8_t bitLength;
} density_lion_entropy_code;

static const density_lion_entropy_code density_lion_form_entropy_codes[DENSITY_LION_NUMBER_OF_FORMS] = DENSITY_LION_FORM_ENTROPY_CODES;
static const density_lion_entropy_code density_lion_index_entropy_codes[32] = DENSITY_LION_INDEX_ENTROPY_CODES;

#endif