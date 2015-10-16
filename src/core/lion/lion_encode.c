/*
 * Centaurean Density
 *
 * Copyright (c) 2015, Guillaume Voirin
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
 * 24/06/15 18:57
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

#include "lion_encode.h"

DENSITY_FORCE_INLINE void density_lion_encode_prepare_signature(uint8_t **restrict out, uint_fast64_t **restrict signature_pointer, uint_fast64_t *const restrict signature) {
    *signature = 0;
    *signature_pointer = (density_lion_signature *) *out;
    *out += sizeof(density_lion_signature);
}

DENSITY_FORCE_INLINE void density_lion_encode_push_to_proximity_signature(uint_fast64_t *const restrict signature, uint_fast8_t *const restrict shift, const uint64_t content, const uint_fast8_t bits) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    *signature |= (content << *shift);
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    *signature |= (content << ((56 - (*shift & ~0x7)) + (*shift & 0x7)));
#else
#error Unknow endianness
#endif

    *shift += bits;
}

DENSITY_FORCE_INLINE void density_lion_encode_push_to_signature(uint8_t **restrict out, uint_fast64_t **restrict signature_pointer, uint_fast64_t *const restrict signature, uint_fast8_t *const restrict shift, const uint64_t content, const uint_fast8_t bits) {
    if (density_likely(*shift)) {
        density_lion_encode_push_to_proximity_signature(signature, shift, content, bits);

        if (density_unlikely(*shift >= density_bitsizeof(density_lion_signature))) {
            DENSITY_MEMCPY(*signature_pointer, signature, sizeof(density_lion_signature));

            const uint_fast8_t remainder = (uint_fast8_t)(*shift & 0x3f);
            *shift = 0;
            if (remainder) {
                density_lion_encode_prepare_signature(out, signature_pointer, signature);
                density_lion_encode_push_to_proximity_signature(signature, shift, content >> (bits - remainder), remainder);
            }
        }
    } else {
        density_lion_encode_prepare_signature(out, signature_pointer, signature);
        density_lion_encode_push_to_proximity_signature(signature, shift, content, bits);
    }
}

DENSITY_FORCE_INLINE void density_lion_encode_push_zero_to_signature(uint8_t **restrict out, uint_fast64_t **restrict signature_pointer, uint_fast64_t *const restrict signature, uint_fast8_t *const restrict shift, const uint_fast8_t bits) {
    if (density_likely(*shift)) {
        *shift += bits;

        if (density_unlikely(*shift >= density_bitsizeof(density_lion_signature))) {
            DENSITY_MEMCPY(*signature_pointer, signature, sizeof(density_lion_signature));

            const uint_fast8_t remainder = (uint_fast8_t)(*shift & 0x3f);
            if (remainder) {
                density_lion_encode_prepare_signature(out, signature_pointer, signature);
                *shift = remainder;
            } else
                *shift = 0;
        }
    } else {
        density_lion_encode_prepare_signature(out, signature_pointer, signature);
        *shift = bits;
    }
}

DENSITY_FORCE_INLINE void density_lion_encode_push_code_to_signature(uint8_t **restrict out, uint_fast64_t **restrict signature_pointer, uint_fast64_t *const restrict signature, uint_fast8_t *const restrict shift, const density_lion_entropy_code code) {
    density_lion_encode_push_to_signature(out, signature_pointer, signature, shift, code.value, code.bitLength);
}

DENSITY_FORCE_INLINE void density_lion_encode_kernel_4(uint8_t **restrict out, uint_fast16_t *restrict last_hash, uint_fast64_t **restrict signature_pointer, uint_fast64_t *const restrict signature, uint_fast8_t *const restrict shift, density_lion_dictionary *const restrict dictionary, const uint16_t hash, density_lion_form_data *const data, const uint32_t unit) {
    density_lion_dictionary_chunk_prediction_entry *const predictions = &dictionary->predictions[*last_hash];
    __builtin_prefetch(&dictionary->predictions[hash]);

    if (*(uint32_t *) predictions ^ unit) {
        if (*((uint32_t *) predictions + 1) ^ unit) {
            if (*((uint32_t *) predictions + 2) ^ unit) {
                density_lion_dictionary_chunk_entry *const in_dictionary = &dictionary->chunks[hash];
                if (*(uint32_t *) in_dictionary ^ unit) {
                    if (*((uint32_t *) in_dictionary + 1) ^ unit) {
                        if (*((uint32_t *) in_dictionary + 2) ^ unit) {
                            if (*((uint32_t *) in_dictionary + 3) ^ unit) {
                                density_lion_encode_push_code_to_signature(out, signature_pointer, signature, shift, density_lion_form_model_get_encoding(data, DENSITY_LION_FORM_PLAIN));
                                DENSITY_MEMCPY(*out, &unit, sizeof(uint32_t));
                                *out += sizeof(uint32_t);
                            } else {
                                density_lion_encode_push_code_to_signature(out, signature_pointer, signature, shift, density_lion_form_model_get_encoding(data, DENSITY_LION_FORM_DICTIONARY_D));
                                DENSITY_MEMCPY(*out, &hash, sizeof(uint16_t));
                                *out += sizeof(uint16_t);
                            }
                        } else {
                            density_lion_encode_push_code_to_signature(out, signature_pointer, signature, shift, density_lion_form_model_get_encoding(data, DENSITY_LION_FORM_DICTIONARY_C));
                            DENSITY_MEMCPY(*out, &hash, sizeof(uint16_t));
                            *out += sizeof(uint16_t);
                        }
                    } else {
                        density_lion_encode_push_code_to_signature(out, signature_pointer, signature, shift, density_lion_form_model_get_encoding(data, DENSITY_LION_FORM_DICTIONARY_B));
                        DENSITY_MEMCPY(*out, &hash, sizeof(uint16_t));
                        *out += sizeof(uint16_t);
                    }
                    DENSITY_MEMMOVE((uint32_t *) in_dictionary + 1, in_dictionary, 3 * sizeof(uint32_t));
                    *(uint32_t *) in_dictionary = unit;
                } else {
                    density_lion_encode_push_code_to_signature(out, signature_pointer, signature, shift, density_lion_form_model_get_encoding(data, DENSITY_LION_FORM_DICTIONARY_A));
                    DENSITY_MEMCPY(*out, &hash, sizeof(uint16_t));
                    *out += sizeof(uint16_t);
                }
            } else {
                density_lion_encode_push_code_to_signature(out, signature_pointer, signature, shift, density_lion_form_model_get_encoding(data, DENSITY_LION_FORM_PREDICTIONS_C));
            }
        } else {
            density_lion_encode_push_code_to_signature(out, signature_pointer, signature, shift, density_lion_form_model_get_encoding(data, DENSITY_LION_FORM_PREDICTIONS_B));
        }
        DENSITY_MEMMOVE((uint32_t *) predictions + 1, predictions, 2 * sizeof(uint32_t));
        *(uint32_t *) predictions = unit;
    } else
        density_lion_encode_push_code_to_signature(out, signature_pointer, signature, shift, density_lion_form_model_get_encoding(data, DENSITY_LION_FORM_PREDICTIONS_A));
    *last_hash = hash;
}

DENSITY_FORCE_INLINE void density_lion_encode_4(const uint8_t **restrict in, uint8_t **restrict out, uint_fast16_t *restrict last_hash, uint_fast64_t **restrict signature_pointer, uint_fast64_t *const restrict signature, uint_fast8_t *const restrict shift, density_lion_dictionary *const restrict dictionary, density_lion_form_data *const data, uint32_t *restrict unit) {
    DENSITY_MEMCPY(unit, *in, sizeof(uint32_t));
    density_lion_encode_kernel_4(out, last_hash, signature_pointer, signature, shift, dictionary, DENSITY_LION_HASH_ALGORITHM(*unit), data, *unit);
    *in += sizeof(uint32_t);
}

DENSITY_FORCE_INLINE void density_lion_encode_generic(const uint8_t **restrict in, uint8_t **restrict out, uint_fast16_t *restrict last_hash, uint_fast64_t **restrict signature_pointer, uint_fast64_t *const restrict signature, uint_fast8_t *const restrict shift, density_lion_dictionary *const restrict dictionary, const uint_fast8_t chunks_per_process_unit, density_lion_form_data *const data, uint32_t *restrict unit) {
#ifdef __clang__
    for (uint_fast8_t count = 0; count < (chunks_per_process_unit >> 2); count++) {
        DENSITY_UNROLL_4(density_lion_encode_4(in, out, last_hash, signature_pointer, signature, shift, dictionary, data, unit));
    }
#else
    for (uint_fast8_t count = 0; count < (chunks_per_process_unit >> 1); count++) {
        DENSITY_UNROLL_2(density_lion_encode_4(in, out, last_hash, signature_pointer, signature, shift, dictionary, data, unit));
    }
#endif
}

DENSITY_FORCE_INLINE void density_lion_encode_32(const uint8_t **restrict in, uint8_t **restrict out, uint_fast16_t *restrict last_hash, uint_fast64_t **restrict signature_pointer, uint_fast64_t *const restrict signature, uint_fast8_t *const restrict shift, density_lion_dictionary *const restrict dictionary, density_lion_form_data *const data, uint32_t *restrict unit) {
    density_lion_encode_generic(in, out, last_hash, signature_pointer, signature, shift, dictionary, DENSITY_LION_CHUNKS_PER_PROCESS_UNIT_SMALL, data, unit);
}

DENSITY_FORCE_INLINE void density_lion_encode_256(const uint8_t **restrict in, uint8_t **restrict out, uint_fast16_t *restrict last_hash, uint_fast64_t **restrict signature_pointer, uint_fast64_t *const restrict signature, uint_fast8_t *const restrict shift, density_lion_dictionary *const restrict dictionary, density_lion_form_data *const data, uint32_t *restrict unit) {
    density_lion_encode_generic(in, out, last_hash, signature_pointer, signature, shift, dictionary, DENSITY_LION_CHUNKS_PER_PROCESS_UNIT_BIG, data, unit);
}

DENSITY_FORCE_INLINE void density_lion_encode_coarse_unrestricted(const uint8_t **restrict in, const uint_fast64_t in_size, uint8_t **restrict out, uint_fast16_t *restrict last_hash, uint_fast64_t **restrict signature_pointer, uint_fast64_t *const restrict signature, uint_fast8_t *const restrict shift, density_lion_form_data *const data, density_lion_dictionary *const restrict dictionary) {

}

DENSITY_FORCE_INLINE void density_lion_encode_fine_unrestricted(const uint8_t **restrict in, const uint_fast64_t in_size, uint8_t **restrict out, uint_fast16_t *restrict last_hash, uint_fast64_t **restrict signature_pointer, uint_fast64_t *const restrict signature, uint_fast8_t *const restrict shift, density_lion_form_data *const data, density_lion_dictionary *const restrict dictionary) {

}

DENSITY_WINDOWS_EXPORT DENSITY_FORCE_INLINE const density_algorithm_exit_status density_lion_encode(density_algorithm_state *const restrict state, const uint8_t **restrict in, const uint_fast64_t in_size, uint8_t **restrict out, const uint_fast64_t out_size, const bool process_all) {
    if (out_size < DENSITY_LION_MAXIMUM_COMPRESSED_UNIT_SIZE)
        return DENSITY_ALGORITHMS_EXIT_STATUS_OUTPUT_STALL;

    density_lion_signature signature;
    density_lion_signature *signature_pointer;
    uint_fast8_t shift = 0;
    density_lion_form_data data;
    density_lion_form_model_init(&data);
    uint_fast16_t last_hash = 0;
    uint32_t unit;

    uint8_t *out_limit = *out + out_size - DENSITY_LION_MAXIMUM_COMPRESSED_UNIT_SIZE;
    uint_fast64_t limit_256 = (in_size >> 8);

    while (density_likely(limit_256-- && *out <= out_limit)) {
        if (density_unlikely(!(state->counter & 0xf))) {
            DENSITY_ALGORITHM_CHECK_USER_INTERRUPT;
            DENSITY_ALGORITHM_REDUCE_COPY_PENALTY_START;
        }
        state->counter++;
        if (density_unlikely(state->copy_penalty)) {
            DENSITY_ALGORITHM_COPY(DENSITY_LION_WORK_BLOCK_SIZE);
            DENSITY_ALGORITHM_INCREASE_COPY_PENALTY_START;
        } else {
            const uint8_t *out_start = *out;
            __builtin_prefetch(*in + DENSITY_LION_WORK_BLOCK_SIZE);
            density_lion_encode_256(in, out, &last_hash, &signature_pointer, &signature, &shift, state->dictionary, &data, &unit);
            DENSITY_ALGORITHM_TEST_INCOMPRESSIBILITY(*out - out_start, DENSITY_LION_WORK_BLOCK_SIZE);
        }
    }

    if (*out > out_limit)
        return DENSITY_ALGORITHMS_EXIT_STATUS_OUTPUT_STALL;

    if (process_all) {
        uint_fast64_t remaining;

        switch (in_size & 0xff) {
            case 0:
            case 1:
            case 2:
            case 3:
                density_lion_encode_push_code_to_signature(out, &signature_pointer, &signature, &shift, density_lion_form_model_get_encoding(&data, DENSITY_LION_FORM_PLAIN)); // End marker
                DENSITY_MEMCPY(signature_pointer, &signature, sizeof(density_lion_signature));
                goto process_remaining_bytes;
            default:
                break;
        }

        uint_fast64_t limit_4 = (in_size & 0xff) >> 2;
        while (limit_4--)
            density_lion_encode_4(in, out, &last_hash, &signature_pointer, &signature, &shift, state->dictionary, &data, &unit);

        density_lion_encode_push_code_to_signature(out, &signature_pointer, &signature, &shift, density_lion_form_model_get_encoding(&data, DENSITY_LION_FORM_PLAIN)); // End marker
        DENSITY_MEMCPY(signature_pointer, &signature, sizeof(density_lion_signature));

        process_remaining_bytes:
        remaining = in_size & 0x3;
        if (remaining) {
            DENSITY_MEMCPY(*out, *in, remaining);
            *in += remaining;
            *out += remaining;
        }
    }

    return DENSITY_ALGORITHMS_EXIT_STATUS_FINISHED;
}