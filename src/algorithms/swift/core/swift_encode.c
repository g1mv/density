/*
 * Centaurean Density
 *
 * Copyright (c) 2018, Guillaume Voirin
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
 * 14/02/18 14:37
 *
 * ---------------
 * Swift algorithm
 * ---------------
 *
 * Author(s)
 * Guillaume Voirin (https://github.com/gpnuma)
 *
 * Description
 * Hash based kernel derived from Chameleon, with very small memory footprint and quicker learning for small datasets
 */

#include "swift_encode.h"

DENSITY_FORCE_INLINE void density_swift_encode_prepare_signature(uint8_t **DENSITY_RESTRICT out, density_swift_signature **DENSITY_RESTRICT signature_pointer, density_swift_signature *const DENSITY_RESTRICT signature) {
    *signature = 0;
    *signature_pointer = (density_swift_signature *) *out;
    *out += sizeof(density_swift_signature);
}

DENSITY_FORCE_INLINE void density_swift_encode_kernel(uint8_t **DENSITY_RESTRICT out, const uint8_t hash, const uint_fast8_t shift, density_swift_signature *const DENSITY_RESTRICT signature, density_swift_dictionary *const DENSITY_RESTRICT dictionary, const uint16_t *DENSITY_RESTRICT unit) {
    density_swift_dictionary_entry *const found = &dictionary->entries[hash];

    switch (*unit ^ found->as_uint16_t) {
        case 0:
            *signature |= ((uint64_t) DENSITY_SWIFT_SIGNATURE_FLAG_MAP << shift);
            DENSITY_MEMCPY(*out, &hash, sizeof(uint8_t));
            *out += sizeof(uint8_t);
            break;
        default:
            found->as_uint16_t = *unit; // Does not ensure dictionary content consistency between endiannesses
            DENSITY_MEMCPY(*out, unit, sizeof(uint16_t));
            *out += sizeof(uint16_t);
            break;
    }
}

DENSITY_FORCE_INLINE const uint32_t density_swift_encode_4(const uint8_t **DENSITY_RESTRICT in, uint8_t **DENSITY_RESTRICT out, const uint_fast8_t shift, density_swift_signature *const DENSITY_RESTRICT signature, density_swift_dictionary *const DENSITY_RESTRICT dictionary, uint32_t *DENSITY_RESTRICT unit) {
    DENSITY_MEMCPY(unit, *in, sizeof(uint32_t));
#ifdef DENSITY_LITTLE_ENDIAN
    const uint16_t a = *unit & 0xffff;
    const uint16_t b = *unit >> 16;
#elif defined(DENSITY_BIG_ENDIAN)
    const uint32_t endian_unit = DENSITY_LITTLE_ENDIAN_32(*unit);
    const uint16_t a = endian_unit & 0xffff;
    const uint16_t b = endian_unit >> 16;
#else
#error
#endif
    const uint32_t mul_value_a = a * DENSITY_SWIFT_HASH_MULTIPLIER;
    const uint32_t mul_value_b = b * DENSITY_SWIFT_HASH_MULTIPLIER;
    density_swift_encode_kernel(out, mul_value_a >> (32 - DENSITY_SWIFT_HASH_BITS), shift, signature, dictionary, &a);
    density_swift_encode_kernel(out, mul_value_b >> (32 - DENSITY_SWIFT_HASH_BITS), shift, signature, dictionary, &b);
    *in += sizeof(uint32_t);

    return mul_value_b + (mul_value_a >> 16);   // Hash used by other algorithms
}
