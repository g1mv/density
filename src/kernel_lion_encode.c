/*
 * Centaurean Density
 *
 * Copyright (c) 2013, Guillaume Voirin
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

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE exitProcess(density_lion_encode_state *state, DENSITY_LION_ENCODE_PROCESS process, DENSITY_KERNEL_ENCODE_STATE kernelEncodeState) {
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


DENSITY_FORCE_INLINE void density_lion_encode_manage_bigram(density_memory_location *restrict out, density_lion_encode_state *restrict state, const uint16_t bigram) {
    const uint8_t hash = DENSITY_LION_BIGRAM_HASH_ALGORITHM(bigram);

    density_lion_dictionary_bigram_entry *bigram_entry = &state->dictionary.bigrams[hash];
    if (bigram_entry->bigram ^ bigram) {
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

DENSITY_FORCE_INLINE void density_lion_encode_kernel(density_memory_location *restrict out, uint32_t *restrict hash, const uint32_t chunk, density_lion_encode_state *restrict state) {
    DENSITY_LION_HASH_ALGORITHM(*hash, DENSITY_LITTLE_ENDIAN_32(chunk));
    density_lion_dictionary_chunk_prediction_entry *p = &(state->dictionary.predictions[state->lastHash]);
    __builtin_prefetch(&(state->dictionary.predictions[*hash]), 1, 3);

    if (*(uint32_t *) p ^ chunk) {
        if (!density_likely(*((uint32_t *) p + 1) ^ chunk)) {
            const density_lion_entropy_code codePb = density_lion_form_model_get_encoding(&state->formData, DENSITY_LION_FORM_CHUNK_SECONDARY_PREDICTIONS);
            density_lion_encode_push_to_signature(out, state, codePb.value, codePb.bitLength + (uint8_t) 1);   // DENSITY_LION_PREDICTIONS_SIGNATURE_FLAG_A
        } else if (!density_likely(*((uint32_t *) p + 2) ^ chunk)) {
            const density_lion_entropy_code codePb = density_lion_form_model_get_encoding(&state->formData, DENSITY_LION_FORM_CHUNK_SECONDARY_PREDICTIONS);
            density_lion_encode_push_to_signature(out, state, codePb.value | (DENSITY_LION_PREDICTIONS_SIGNATURE_FLAG_B << codePb.bitLength), codePb.bitLength + (uint8_t) 1);
        } else {
            density_lion_dictionary_chunk_entry *found = &state->dictionary.chunks[*hash];
            uint32_t *found_a = &found->chunk_a;
            if (*found_a ^ chunk) {
                uint32_t *found_b = &found->chunk_b;
                if (*found_b ^ chunk) {
                    const density_lion_entropy_code codeSA = density_lion_form_model_get_encoding(&state->formData, DENSITY_LION_FORM_SECONDARY_ACCESS);
                    density_lion_encode_push_to_signature(out, state, codeSA.value, codeSA.bitLength);

                    density_lion_encode_manage_bigram(out, state, (uint16_t) (chunk));
                    density_lion_encode_manage_bigram(out, state, (uint16_t) (chunk >> 16));

                    const uint16_t mid_bigram = (uint16_t) (chunk >> 8);
                    state->dictionary.bigrams[DENSITY_LION_BIGRAM_HASH_ALGORITHM(mid_bigram)].bigram = mid_bigram;
                } else {
                    const density_lion_entropy_code codeDB = density_lion_form_model_get_encoding(&state->formData, DENSITY_LION_FORM_CHUNK_DICTIONARY_B);
                    density_lion_encode_push_to_signature(out, state, codeDB.value, codeDB.bitLength);

                    *(uint16_t *) (out->pointer) = DENSITY_LITTLE_ENDIAN_16(*hash);
                    out->pointer += sizeof(uint16_t);
                }
                *found_b = *found_a;
                *found_a = chunk;
            } else {
                const density_lion_entropy_code codeDA = density_lion_form_model_get_encoding(&state->formData, DENSITY_LION_FORM_CHUNK_DICTIONARY_A);
                density_lion_encode_push_to_signature(out, state, codeDA.value, codeDA.bitLength);

                *(uint16_t *) (out->pointer) = DENSITY_LITTLE_ENDIAN_16(*hash);
                out->pointer += sizeof(uint16_t);
            }
        }

        *(uint64_t *) ((uint32_t *) p + 1) = *(uint64_t *) p;
        p->next_chunk_a = chunk;    // Move chunk to the top of the predictions list
    } else {
        const density_lion_entropy_code codePa = density_lion_form_model_get_encoding(&state->formData, DENSITY_LION_FORM_CHUNK_PREDICTIONS);
        density_lion_encode_push_to_signature(out, state, codePa.value, codePa.bitLength);
    }

    state->lastHash = *hash;
    state->lastChunk = chunk;
}

DENSITY_FORCE_INLINE void density_lion_encode_process_chunk(uint64_t *restrict chunk, density_memory_location *restrict in, density_memory_location *restrict out, uint32_t *restrict hash, density_lion_encode_state *restrict state) {
    *chunk = *(uint64_t *) (in->pointer);
    __builtin_prefetch((uint64_t *) (in->pointer) + 1, 0, 3);

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    density_lion_encode_kernel(out, hash, (uint32_t) (*chunk & 0xFFFFFFFF), state);
#endif
    density_lion_encode_kernel(out, hash, (uint32_t) (*chunk >> 32), state);
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    density_lion_encode_kernel(out, hash, (uint32_t) (*chunk & 0xFFFFFFFF), state);
#endif

    in->pointer += sizeof(uint64_t);
}

DENSITY_FORCE_INLINE void density_lion_encode_process_unit(uint64_t *restrict chunk, density_memory_location *restrict in, density_memory_location *restrict out, uint32_t *restrict hash, density_lion_encode_state *restrict state) {
    DENSITY_UNROLL_2(density_lion_encode_process_chunk(chunk, in, out, hash, state));

    state->chunksCount += DENSITY_LION_CHUNKS_PER_PROCESS_UNIT;
}

DENSITY_FORCE_INLINE bool density_lion_encode_process_unit_with_intercept(uint64_t *restrict chunk, density_memory_location *restrict in, density_memory_location *restrict out, uint32_t *restrict hash, density_lion_encode_state *restrict state) {
    const uint_fast32_t start_shift = state->shift;

    DENSITY_UNROLL_2(density_lion_encode_process_chunk(chunk, in, out, hash, state));

    state->chunksCount += DENSITY_LION_CHUNKS_PER_PROCESS_UNIT;

    if(start_shift > state->shift) {    // New signature has been created
        const density_byte* content_start = (density_byte*)state->signature + sizeof(density_lion_signature);
        state->transientContent.content = *(uint64_t*)content_start;
        state->transientContent.size = (uint8_t)(out->pointer - content_start);
        return true;
    } else
        return false;
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE density_lion_encode_init(density_lion_encode_state *state) {
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

    return exitProcess(state, DENSITY_LION_ENCODE_PROCESS_CHECK_BLOCK_STATE, DENSITY_KERNEL_ENCODE_STATE_READY);
}

#define DENSITY_LION_ENCODE_CONTINUE

#include "kernel_lion_encode_template.h"

#undef DENSITY_LION_ENCODE_CONTINUE

#include "kernel_lion_encode_template.h"
