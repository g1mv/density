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

#include "globals.h"

#define DENSITY_LION_PREFERRED_BLOCK_CHUNKS_SHIFT                   17
#define DENSITY_LION_PREFERRED_BLOCK_CHUNKS                         (1 << DENSITY_LION_PREFERRED_BLOCK_CHUNKS_SHIFT)

#define DENSITY_LION_PREFERRED_EFFICIENCY_CHECK_CHUNKS_SHIFT        13
#define DENSITY_LION_PREFERRED_EFFICIENCY_CHECK_CHUNKS              (1 << DENSITY_LION_PREFERRED_EFFICIENCY_CHECK_CHUNKS_SHIFT)

#define DENSITY_LION_HASH32_MULTIPLIER                                  (uint32_t)0x9D6EF916lu
#define DENSITY_LION_HASH16_MULTIPLIER                                  (uint32_t)0xAD15lu
#define DENSITY_LION_CHUNK_HASH_BITS                                    16
#define DENSITY_LION_BIGRAM_HASH_BITS                                   8

#define DENSITY_LION_HASH_ALGORITHM(hash32, value32)                    hash32 = value32 * DENSITY_LION_HASH32_MULTIPLIER;\
                                                                        hash32 = (hash32 >> (32 - DENSITY_LION_CHUNK_HASH_BITS));

#define DENSITY_LION_BIGRAM_HASH_ALGORITHM(bigram)                      (uint8_t) ((bigram * DENSITY_LION_HASH16_MULTIPLIER) >> (16 - DENSITY_LION_BIGRAM_HASH_BITS))

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

#define DENSITY_LION_MAXIMUM_COMPRESSED_UNIT_SIZE                       (sizeof(density_lion_signature) + DENSITY_LION_MAXIMUM_COMPRESSED_BODY_SIZE_PER_SIGNATURE)
#define DENSITY_LION_MAXIMUM_DECOMPRESSED_UNIT_SIZE                     (DENSITY_LION_MAXIMUM_DECOMPRESSED_BODY_SIZE_PER_SIGNATURE)

#define DENSITY_LION_SIGNATURE_SHIFT_LIMIT                              (0x3C)

#define DENSITY_LION_CHUNKS_PER_PROCESS_UNIT                            4
#define DENSITY_LION_PROCESS_UNIT_SIZE                                  (DENSITY_LION_CHUNKS_PER_PROCESS_UNIT * sizeof(uint32_t))

#endif
