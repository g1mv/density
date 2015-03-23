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

#define DENSITY_LION_PREFERRED_BLOCK_CHUNKS_SHIFT                   17
#define DENSITY_LION_PREFERRED_BLOCK_CHUNKS                         (1 << DENSITY_LION_PREFERRED_BLOCK_CHUNKS_SHIFT)

#define DENSITY_LION_PREFERRED_EFFICIENCY_CHECK_CHUNKS_SHIFT        13
#define DENSITY_LION_PREFERRED_EFFICIENCY_CHECK_CHUNKS              (1 << DENSITY_LION_PREFERRED_EFFICIENCY_CHECK_CHUNKS_SHIFT)

#define DENSITY_LION_HASH32_MULTIPLIER                                  (uint32_t)0x9D6EF916lu
#define DENSITY_LION_CHUNK_HASH_BITS                                    16
#define DENSITY_LION_BIGRAM_HASH_BITS                                   8

#define DENSITY_LION_HASH_ALGORITHM(hash32, value32)                    hash32 = value32 * DENSITY_LION_HASH32_MULTIPLIER;\
                                                                        hash32 = (hash32 >> (32 - DENSITY_LION_CHUNK_HASH_BITS));

#define DENSITY_LION_BIGRAM_HASH_ALGORITHM(bigram)                      (uint8_t) ((((bigram) * DENSITY_LION_HASH32_MULTIPLIER) >> (32 - DENSITY_LION_BIGRAM_HASH_BITS)))

#define DENSITY_FORMAT(v)               0##v##llu

#define DENSITY_ISOLATE(b, p)           ((DENSITY_FORMAT(b) / p) & 0x1)

#define DENSITY_BINARY_TO_UINT(b)        ((DENSITY_ISOLATE(b, 1llu) ? 0x1 : 0)\
                                        + (DENSITY_ISOLATE(b, 8llu) ? 0x2 : 0)\
                                        + (DENSITY_ISOLATE(b, 64llu) ? 0x4 : 0)\
                                        + (DENSITY_ISOLATE(b, 512llu) ? 0x8 : 0)\
                                        + (DENSITY_ISOLATE(b, 4096llu) ? 0x10 : 0)\
                                        + (DENSITY_ISOLATE(b, 32768llu) ? 0x20 : 0)\
                                        + (DENSITY_ISOLATE(b, 262144llu) ? 0x40 : 0)\
                                        + (DENSITY_ISOLATE(b, 2097152llu) ? 0x80 : 0)\
                                        + (DENSITY_ISOLATE(b, 16777216llu) ? 0x100 : 0)\
                                        + (DENSITY_ISOLATE(b, 134217728llu) ? 0x200 : 0)\
                                        + (DENSITY_ISOLATE(b, 1073741824llu) ? 0x400 : 0)\
                                        + (DENSITY_ISOLATE(b, 8589934592llu) ? 0x800 : 0)\
                                        + (DENSITY_ISOLATE(b, 68719476736llu) ? 0x1000 : 0)\
                                        + (DENSITY_ISOLATE(b, 549755813888llu) ? 0x2000 : 0)\
                                        + (DENSITY_ISOLATE(b, 4398046511104llu) ? 0x4000 : 0)\
                                        + (DENSITY_ISOLATE(b, 35184372088832llu) ? 0x8000 : 0)\
                                        + (DENSITY_ISOLATE(b, 281474976710656llu) ? 0x10000 : 0)\
                                        + (DENSITY_ISOLATE(b, 2251799813685248llu) ? 0x20000 : 0))

/*#define S1(x)           DENSITY_LION_BIGRAM_HASH_ALGORITHM(x)
#define S4(x)           S1(x),          S1(x + 1),          S1(x + 2),          S1(x + 3)
#define S16(x)          S4(x),          S4(x + 4),          S4(x + 8),          S4(x + 12)
#define S64(x)          S16(x),         S16(x + 16),        S16(x + 32),        S16(x + 48)
#define S256(x)         S64(x),         S64(x + 64),        S64(x + 128),       S64(x + 192)
#define S1024(x)        S256(x),        S256(x + 256),      S256(x + 512),      S256(x + 768)
#define S4096(x)        S1024(x),       S1024(x + 1024),    S1024(x + 2048),    S1024(x + 3072)
#define S16384(x)       S4096(x),       S4096(x + 4096),    S4096(x + 8192),    S4096(x + 12288)
#define S65536(x)       S16384(x),      S16384(x + 16384),  S16384(x + 32768),  S4096(x + 49152)
static const uint8_t DENSITY_LION_BIGRAM_HASH_LOOKUP_TABLE[1 << 16] = {S65536(0)};*/

typedef enum {
    DENSITY_LION_FORM_CHUNK_PREDICTIONS,
    DENSITY_LION_FORM_CHUNK_SECONDARY_PREDICTIONS,
    DENSITY_LION_FORM_CHUNK_DICTIONARY_A,
    DENSITY_LION_FORM_CHUNK_DICTIONARY_B,
    DENSITY_LION_FORM_SECONDARY_ACCESS,
} DENSITY_LION_FORM;

typedef enum {
    DENSITY_LION_BIGRAM_SIGNATURE_FLAG_DICTIONARY = 0x0,
    DENSITY_LION_BIGRAM_SIGNATURE_FLAG_PLAIN = 0x1,
} DENSITY_LION_BIGRAM_SIGNATURE_FLAG;

typedef enum {
    DENSITY_LION_PREDICTIONS_SIGNATURE_FLAG_A = 0x0,
    DENSITY_LION_PREDICTIONS_SIGNATURE_FLAG_B = 0x1,
} DENSITY_LION_PREDICTIONS_SIGNATURE_FLAG;

#pragma pack(push)
#pragma pack(4)
typedef struct {
    uint_fast8_t value;
    uint_fast8_t bitLength;
} density_lion_entropy_code;
#pragma pack(pop)

typedef uint64_t                                                        density_lion_signature;

#define DENSITY_LION_MAXIMUM_COMPRESSED_BODY_SIZE_PER_SIGNATURE         (density_bitsizeof(density_lion_signature) * sizeof(uint16_t))   // Dictionary * hash
#define DENSITY_LION_MAXIMUM_DECOMPRESSED_BODY_SIZE_PER_SIGNATURE       (density_bitsizeof(density_lion_signature) * sizeof(uint32_t))   // Predictions * chunk

#define DENSITY_LION_MAXIMUM_COMPRESSED_UNIT_SIZE                       (density_bitsizeof(density_lion_signature) + DENSITY_LION_MAXIMUM_COMPRESSED_BODY_SIZE_PER_SIGNATURE)
#define DENSITY_LION_MAXIMUM_DECOMPRESSED_UNIT_SIZE                     (DENSITY_LION_MAXIMUM_DECOMPRESSED_BODY_SIZE_PER_SIGNATURE)

#define DENSITY_LION_SIGNATURE_SHIFT_LIMIT                              (0x3C)

#define DENSITY_LION_CHUNKS_PER_PROCESS_UNIT                            4
#define DENSITY_LION_PROCESS_UNIT_SIZE                                  (DENSITY_LION_CHUNKS_PER_PROCESS_UNIT * sizeof(uint32_t))

#endif
