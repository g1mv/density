/*
 * Centaurean Density
 *
 * Copyright (c) 2013, Guillaume Voirin
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     1. Redistributions of source code must retain the above copyright notice, this
 *        list of conditions and the following disclaimer.
 *
 *     2. Redistributions in binary form must reproduce the above copyright notice,
 *        this list of conditions and the following disclaimer in the documentation
 *        and/or other materials provided with the distribution.
 *
 *     3. Neither the name of the copyright holder nor the names of its
 *        contributors may be used to endorse or promote products derived from
 *        this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * 23/06/15 23:29
 *
 * -----------------
 * Cheetah algorithm
 * -----------------
 *
 * Author(s)
 * Guillaume Voirin (https://github.com/gpnuma)
 * Piotr Tarsa (https://github.com/tarsa)
 *
 * Description
 * Very fast two level dictionary hash algorithm derived from Chameleon, with predictions lookup
 */

#include "kernel_cheetah_encode_bulk.h"

DENSITY_FORCE_INLINE void density_cheetah_encode_bulk_prepare_signature(uint8_t **restrict out, uint_fast64_t **restrict signature_pointer, uint_fast64_t *const restrict signature) {
    *signature = 0;
    *signature_pointer = (density_cheetah_signature *) *out;
    *out += sizeof(density_cheetah_signature);
}

DENSITY_FORCE_INLINE void density_cheetah_encode_bulk_kernel(uint8_t **restrict out, uint_fast16_t *restrict last_hash, const uint_fast16_t hash, const uint_fast8_t shift, uint_fast64_t *const restrict signature, density_cheetah_dictionary *const restrict dictionary, uint32_t *restrict unit) {
    uint32_t *predictedChunk = (uint32_t *) &dictionary->prediction_entries[*last_hash];

    if (*predictedChunk ^ *unit) {
        density_cheetah_dictionary_entry *found = &dictionary->entries[hash];
        uint32_t *found_a = &found->chunk_a;
        if (*found_a ^ *unit) {
            uint32_t *found_b = &found->chunk_b;
            if (*found_b ^ *unit) {
                *signature |= ((uint64_t) DENSITY_CHEETAH_SIGNATURE_FLAG_CHUNK << shift);
                DENSITY_MEMCPY(*out, unit, sizeof(uint32_t));
                *out += sizeof(uint32_t);
            } else {
                *signature |= ((uint64_t) DENSITY_CHEETAH_SIGNATURE_FLAG_MAP_B << shift);
                DENSITY_MEMCPY(*out, &hash, sizeof(uint16_t));
                *out += sizeof(uint16_t);
            }
            *found_b = *found_a;
            *found_a = *unit;
        } else {
            *signature |= ((uint64_t) DENSITY_CHEETAH_SIGNATURE_FLAG_MAP_A << shift);
            DENSITY_MEMCPY(*out, &hash, sizeof(uint16_t));
            *out += sizeof(uint16_t);
        }
        *predictedChunk = *unit;
    }
    *last_hash = hash;
}

DENSITY_FORCE_INLINE void density_cheetah_encode_bulk_4(const uint8_t **restrict in, uint8_t **restrict out, uint_fast16_t *restrict last_hash, const uint_fast8_t shift, uint_fast64_t *const restrict signature, density_cheetah_dictionary *const restrict dictionary, uint32_t *restrict unit) {
    DENSITY_MEMCPY(unit, *in, sizeof(uint32_t));
    *in += sizeof(uint32_t);
    density_cheetah_encode_bulk_kernel(out, last_hash, DENSITY_CHEETAH_HASH_ALGORITHM(*unit), shift, signature, dictionary, unit);
}

DENSITY_FORCE_INLINE void density_cheetah_encode_bulk_128(const uint8_t **restrict in, uint8_t **restrict out, uint_fast16_t *restrict last_hash, uint_fast64_t *const restrict signature, density_cheetah_dictionary *const restrict dictionary, uint32_t *restrict unit) {
    uint_fast8_t count = 0;

#ifdef __clang__
    for(; count < density_bitsizeof(density_cheetah_signature); count += 2) {
        density_cheetah_encode_bulk_4(in, out, last_hash, count, signature, dictionary, unit);
    }
#else
    for (uint_fast8_t count_b = 0; count_b < 16; count_b++) {
        DENSITY_UNROLL_2(\
        density_cheetah_encode_bulk_4(in, out, last_hash, count, signature, dictionary, unit);\
        count += 2);
    }
#endif
}

DENSITY_FORCE_INLINE void density_cheetah_encode_bulk_coarse_unrestricted(const uint8_t **restrict in, const uint_fast64_t in_size, uint8_t **restrict out, uint_fast16_t *restrict last_hash, uint_fast64_t **restrict signature_pointer, uint_fast64_t *const restrict signature, density_cheetah_dictionary *const restrict dictionary) {
    uint32_t unit;

    uint_fast64_t limit_128 = (in_size >> 7);
    while (limit_128--) {
        density_cheetah_encode_bulk_prepare_signature(out, signature_pointer, signature);
        __builtin_prefetch(*in + 128);
        density_cheetah_encode_bulk_128(in, out, last_hash, signature, dictionary, &unit);
        DENSITY_MEMCPY(*signature_pointer, signature, sizeof(density_cheetah_signature));
    }
}

DENSITY_FORCE_INLINE void density_cheetah_encode_bulk_fine_unrestricted(const uint8_t **restrict in, const uint_fast64_t in_size, uint8_t **restrict out, uint_fast16_t *restrict last_hash, uint_fast64_t **restrict signature_pointer, uint_fast64_t *const restrict signature, density_cheetah_dictionary *const restrict dictionary) {
    uint32_t unit;
    uint_fast64_t remaining;

    switch (in_size & 0x7f) {
        case 0:
        case 1:
        case 2:
        case 3:
            density_cheetah_encode_bulk_prepare_signature(out, signature_pointer, signature);
            *signature |= ((uint64_t) DENSITY_CHEETAH_SIGNATURE_FLAG_CHUNK);   // End marker
            DENSITY_MEMCPY(*signature_pointer, signature, sizeof(density_cheetah_signature));
            goto process_remaining_bytes;
        default:
            break;
    }

    const uint_fast64_t limit_4 = ((in_size & 0x7f) >> 2) << 1; // 4-byte units times number of signature flag bits
    density_cheetah_encode_bulk_prepare_signature(out, signature_pointer, signature);
    for (uint_fast8_t shift = 0; shift != limit_4; shift += 2)
        density_cheetah_encode_bulk_4(in, out, last_hash, shift, signature, dictionary, &unit);

    *signature |= ((uint64_t) DENSITY_CHEETAH_SIGNATURE_FLAG_CHUNK << limit_4);   // End marker
    DENSITY_MEMCPY(*signature_pointer, signature, sizeof(density_cheetah_signature));

    process_remaining_bytes:
    remaining = in_size & 0x3;
    if (remaining) {
        DENSITY_MEMCPY(*out, *in, remaining);
        *in += remaining;
        *out += remaining;
    }
}

DENSITY_FORCE_INLINE void density_cheetah_encode_bulk_unrestricted(const uint8_t **restrict in, const uint_fast64_t in_size, uint8_t **restrict out) {
    density_cheetah_signature signature;
    density_cheetah_signature *signature_pointer;
    density_cheetah_dictionary dictionary;
    density_cheetah_dictionary_reset(&dictionary);
    uint_fast16_t last_hash = 0;

    density_cheetah_encode_bulk_coarse_unrestricted(in, in_size, out, &last_hash, &signature_pointer, &signature, &dictionary);
    density_cheetah_encode_bulk_fine_unrestricted(in, in_size, out, &last_hash, &signature_pointer, &signature, &dictionary);
}