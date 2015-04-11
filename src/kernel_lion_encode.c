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
 * 06/12/13 20:28
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

#include "kernel_lion_encode.h"

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE density_lion_encode_exit_process(density_lion_encode_state *state, DENSITY_LION_ENCODE_PROCESS process, DENSITY_KERNEL_ENCODE_STATE kernelEncodeState) {
    state->process = process;
    return kernelEncodeState;
}

DENSITY_FORCE_INLINE void density_lion_encode_prepare_new_signature(density_memory_location *restrict out, density_lion_encode_state *restrict state) {
    state->signature = (density_lion_signature *) (out->pointer);
    state->proximitySignature = 0;

    out->pointer += sizeof(density_lion_signature);
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE density_lion_encode_check_block_state(density_lion_encode_state *restrict state) {
    if (density_unlikely((state->chunksCount >= DENSITY_LION_PREFERRED_EFFICIENCY_CHECK_CHUNKS) && (!state->efficiencyChecked))) {
        state->efficiencyChecked = true;
        return DENSITY_KERNEL_ENCODE_STATE_INFO_EFFICIENCY_CHECK;
    } else if (density_unlikely(state->chunksCount >= DENSITY_LION_PREFERRED_BLOCK_CHUNKS)) {
        state->chunksCount = 0;
        state->efficiencyChecked = false;

#if DENSITY_ENABLE_PARALLELIZABLE_DECOMPRESSIBLE_OUTPUT == DENSITY_YES
            if (state->resetCycle)
                state->resetCycle--;
            else {
                density_lion_dictionary_reset(&state->dictionary);
                state->resetCycle = DENSITY_DICTIONARY_PREFERRED_RESET_CYCLE - 1;
            }
#endif

        return DENSITY_KERNEL_ENCODE_STATE_INFO_NEW_BLOCK;
    }

    return DENSITY_KERNEL_ENCODE_STATE_READY;
}

DENSITY_FORCE_INLINE void density_lion_encode_push_to_proximity_signature(density_lion_encode_state *restrict state, const uint64_t content, const uint_fast8_t bits) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    state->proximitySignature |= (content << state->shift);
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    state->proximitySignature |= (content << ((56 - (state->shift & ~0x7)) + (state->shift & 0x7)));
#else
#error Unknow endianness
#endif

    state->shift += bits;
}

DENSITY_FORCE_INLINE void density_lion_encode_push_to_signature(density_memory_location *restrict out, density_lion_encode_state *restrict state, const uint64_t content, const uint_fast8_t bits) {
    if (density_likely(state->shift)) {
        density_lion_encode_push_to_proximity_signature(state, content, bits);

        if (density_unlikely(state->shift >= density_bitsizeof(density_lion_signature))) {
            *state->signature = state->proximitySignature;

            const uint_fast8_t remainder = (uint_fast8_t) (state->shift & 0x3F);
            state->shift = 0;
            if (remainder) {
                density_lion_encode_prepare_new_signature(out, state);
                density_lion_encode_push_to_proximity_signature(state, content >> (bits - remainder), remainder);
            }
        }
    } else {
        density_lion_encode_prepare_new_signature(out, state);
        density_lion_encode_push_to_proximity_signature(state, content, bits);
    }
}

DENSITY_FORCE_INLINE void density_lion_encode_push_zero_to_signature(density_memory_location *restrict out, density_lion_encode_state *restrict state, const uint_fast8_t bits) {
    if (density_likely(state->shift)) {
        state->shift += bits;

        if (density_unlikely(state->shift >= density_bitsizeof(density_lion_signature))) {
            *state->signature = state->proximitySignature;

            const uint_fast8_t remainder = (uint_fast8_t) (state->shift & 0x3F);
            if (remainder) {
                density_lion_encode_prepare_new_signature(out, state);
                state->shift = remainder;
            } else
                state->shift = 0;
        }
    } else {
        density_lion_encode_prepare_new_signature(out, state);
        state->shift = bits;
    }
}

DENSITY_FORCE_INLINE void density_lion_encode_push_code_to_signature(density_memory_location *restrict out, density_lion_encode_state *restrict state, const density_lion_entropy_code code) {
    density_lion_encode_push_to_signature(out, state, code.value, code.bitLength);
}


DENSITY_FORCE_INLINE void density_lion_encode_manage_bigram(density_memory_location *restrict out, density_lion_encode_state *restrict state, const uint8_t hash, const uint16_t bigram) {
    density_lion_dictionary_bigram_entry *bigram_entry = &state->dictionary.bigrams[hash];
    if (bigram_entry->bigram != bigram) {
        density_lion_encode_push_to_signature(out, state, DENSITY_LION_BIGRAM_SIGNATURE_FLAG_PLAIN, 1);

        *(uint16_t *) out->pointer = DENSITY_LITTLE_ENDIAN_16(bigram);
        out->pointer += sizeof(uint16_t);

        bigram_entry->bigram = bigram;
    } else {
        density_lion_encode_push_zero_to_signature(out, state, 1);  // DENSITY_LION_BIGRAM_SIGNATURE_FLAG_DICTIONARY

        *(out->pointer) = hash;
        out->pointer += sizeof(uint8_t);
    }
}

DENSITY_FORCE_INLINE void density_lion_encode_kernel(density_memory_location *restrict out, const uint16_t hash, const uint32_t chunk, density_lion_encode_state *restrict state) {
    density_lion_dictionary_chunk_prediction_entry *p = &(state->dictionary.predictions[state->lastHash]);
    __builtin_prefetch(&(state->dictionary.predictions[hash]), 1, 3);

    if (*(uint32_t *) p != chunk) {
        if (!density_likely(*((uint32_t *) p + 1) != chunk)) {
            const density_lion_entropy_code codePb = density_lion_form_model_get_encoding(&state->formData, DENSITY_LION_FORM_CHUNK_SECONDARY_PREDICTIONS);
            density_lion_encode_push_to_signature(out, state, codePb.value, codePb.bitLength + (uint8_t) 1);   // DENSITY_LION_PREDICTIONS_SIGNATURE_FLAG_A
        } else if (!density_likely(*((uint32_t *) p + 2) != chunk)) {
            const density_lion_entropy_code codePb = density_lion_form_model_get_encoding(&state->formData, DENSITY_LION_FORM_CHUNK_SECONDARY_PREDICTIONS);
            density_lion_encode_push_to_signature(out, state, codePb.value | (DENSITY_LION_PREDICTIONS_SIGNATURE_FLAG_B << codePb.bitLength), codePb.bitLength + (uint8_t) 1);
        } else {
            density_lion_dictionary_chunk_entry *found = &state->dictionary.chunks[hash];
            uint32_t *found_a = &found->chunk_a;
            if (*found_a ^ chunk) {
                uint32_t *found_b = &found->chunk_b;
                if (*found_b ^ chunk) {
                    density_lion_encode_push_code_to_signature(out, state, density_lion_form_model_get_encoding(&state->formData, DENSITY_LION_FORM_SECONDARY_ACCESS));

                    const uint32_t hash_group = (uint16_t)((uint16_t)chunk * DENSITY_LION_HASH16_MULTIPLIER) | ((chunk & DENSITY_MASK_16_32) * DENSITY_LION_HASH16_MULTIPLIER);
                    density_lion_encode_manage_bigram(out, state, *((uint8_t *) &hash_group + 1), (uint16_t)chunk);
                    density_lion_encode_manage_bigram(out, state, *((uint8_t *) &hash_group + 3), *((uint16_t *) &chunk + 1));
                } else {
                    density_lion_encode_push_code_to_signature(out, state, density_lion_form_model_get_encoding(&state->formData, DENSITY_LION_FORM_CHUNK_DICTIONARY_B));

                    *(uint16_t *) (out->pointer) = DENSITY_LITTLE_ENDIAN_16(hash);
                    out->pointer += sizeof(uint16_t);
                }
                *found_b = *found_a;
                *found_a = chunk;
            } else {
                density_lion_encode_push_code_to_signature(out, state, density_lion_form_model_get_encoding(&state->formData, DENSITY_LION_FORM_CHUNK_DICTIONARY_A));

                *(uint16_t *) (out->pointer) = DENSITY_LITTLE_ENDIAN_16(hash);
                out->pointer += sizeof(uint16_t);
            }
        }

        *(uint64_t *) ((uint32_t *) p + 1) = *(uint64_t *) p;
        p->next_chunk_a = chunk;    // Move chunk to the top of the predictions list
    } else {
        density_lion_encode_push_code_to_signature(out, state, density_lion_form_model_get_encoding(&state->formData, DENSITY_LION_FORM_CHUNK_PREDICTIONS));
    }

    state->lastHash = hash;
    state->lastChunk = chunk;
}

DENSITY_FORCE_INLINE void density_lion_encode_process_chunk(density_memory_location *restrict in, density_memory_location *restrict out, density_lion_encode_state *restrict state) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    const uint128_t chunk = *(uint128_t * )(in->pointer);
    const uint128_t hash_group = (uint32_t) (((uint32_t) chunk * DENSITY_LION_HASH32_MULTIPLIER)) | (uint64_t) (((uint64_t) (chunk & DENSITY_MASK_32_64) * DENSITY_LION_HASH32_MULTIPLIER)) | (((chunk & DENSITY_MASK_64_96) * DENSITY_LION_HASH32_MULTIPLIER) & DENSITY_MASK_64_96) | (((chunk & DENSITY_MASK_96_128) * DENSITY_LION_HASH32_MULTIPLIER));

    density_lion_encode_kernel(out, *((uint16_t *) &hash_group + 1), (uint32_t) chunk, state);
    density_lion_encode_kernel(out, *((uint16_t *) &hash_group + 3), *((uint32_t *) &chunk + 1), state);
    density_lion_encode_kernel(out, *((uint16_t *) &hash_group + 5), *((uint32_t *) &chunk + 2), state);
    density_lion_encode_kernel(out, *((uint16_t *) &hash_group + 7), *((uint32_t *) &chunk + 3), state);
#else
    const uint64_t chunk_a = *(uint64_t *) (in->pointer);
    const uint32_t element_a = (uint32_t) (chunk_a >> density_bitsizeof(uint32_t));
    const uint32_t element_b = (uint32_t) chunk_a;
    density_lion_encode_kernel(out, DENSITY_LION_HASH_ALGORITHM(DENSITY_LITTLE_ENDIAN_32(element_a)), element_a, state);
    density_lion_encode_kernel(out, DENSITY_LION_HASH_ALGORITHM(DENSITY_LITTLE_ENDIAN_32(element_b)), element_b, state);

    const uint64_t chunk_b = *((uint64_t *) in->pointer + 1);
    const uint32_t element_c = (uint32_t) (chunk_b >> density_bitsizeof(uint32_t));
    const uint32_t element_d = (uint32_t) chunk_b;
    density_lion_encode_kernel(out, DENSITY_LION_HASH_ALGORITHM(DENSITY_LITTLE_ENDIAN_32(element_c)), element_c, state);
    density_lion_encode_kernel(out, DENSITY_LION_HASH_ALGORITHM(DENSITY_LITTLE_ENDIAN_32(element_d)), element_d, state);
#endif

    in->pointer += sizeof(uint128_t);
}

DENSITY_FORCE_INLINE void density_lion_encode_process_unit(density_memory_location *restrict in, density_memory_location *restrict out, density_lion_encode_state *restrict state) {
    density_lion_encode_process_chunk(in, out, state);
    state->chunksCount += DENSITY_LION_CHUNKS_PER_PROCESS_UNIT;

    in->available_bytes -= DENSITY_LION_PROCESS_UNIT_SIZE;
}

DENSITY_FORCE_INLINE void density_lion_encode_process_step_unit(density_memory_location *restrict in, density_memory_location *restrict out, density_lion_encode_state *restrict state) {
    const uint32_t chunk = *(uint32_t *) (in->pointer);
    density_lion_encode_kernel(out, DENSITY_LION_HASH_ALGORITHM(DENSITY_LITTLE_ENDIAN_32(chunk)), chunk, state);
    state->chunksCount++;

    in->pointer += sizeof(uint32_t);
    in->available_bytes -= sizeof(uint32_t);
}

DENSITY_WINDOWS_EXPORT DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE density_lion_encode_init(density_lion_encode_state *state) {
    state->chunksCount = 0;
    state->efficiencyChecked = false;
    state->signature = NULL;
    state->shift = 0;
    density_lion_dictionary_reset(&state->dictionary);

#if DENSITY_ENABLE_PARALLELIZABLE_DECOMPRESSIBLE_OUTPUT == DENSITY_YES
    state->resetCycle = DENSITY_DICTIONARY_PREFERRED_RESET_CYCLE - 1;
#endif

    density_lion_form_model_init(&state->formData);

    state->lastHash = 0;
    state->lastChunk = 0;

    state->signatureInterceptMode = false;
    state->endMarker = false;

    return density_lion_encode_exit_process(state, DENSITY_LION_ENCODE_PROCESS_CHECK_BLOCK_STATE, DENSITY_KERNEL_ENCODE_STATE_READY);
}

#include "kernel_lion_encode_template.h"

#define DENSITY_LION_ENCODE_FINISH

#include "kernel_lion_encode_template.h"
