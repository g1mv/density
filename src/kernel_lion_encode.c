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
#include "kernel_lion.h"

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE exitProcess(density_lion_encode_state *state, DENSITY_LION_ENCODE_PROCESS process, DENSITY_KERNEL_ENCODE_STATE kernelEncodeState) {
    state->process = process;
    return kernelEncodeState;
}

DENSITY_FORCE_INLINE void density_lion_encode_prepare_new_signature(density_memory_location *restrict out, density_lion_encode_state *restrict state) {
    state->signaturesCount++;
    state->shift = 0;
    state->signature = (density_lion_signature *) (out->pointer);
    state->proximitySignature = 0;

    out->pointer += sizeof(density_lion_signature);
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE density_lion_encode_prepare_new_block(density_memory_location *restrict out, density_lion_encode_state *restrict state) {
    if ((state->signaturesCount > DENSITY_LION_PREFERRED_EFFICIENCY_CHECK_SIGNATURES) && (state->efficiencyChecked ^ 0x1)) {
        state->efficiencyChecked = 1;
        return DENSITY_KERNEL_ENCODE_STATE_INFO_EFFICIENCY_CHECK;
    } else if (state->signaturesCount > DENSITY_LION_PREFERRED_BLOCK_SIGNATURES) {
        state->signaturesCount = 0;
        state->efficiencyChecked = 0;

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

    density_lion_encode_prepare_new_signature(out, state);

    return DENSITY_KERNEL_ENCODE_STATE_READY;
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE density_lion_encode_check_state(density_memory_location *restrict out, density_lion_encode_state *restrict state) {
    DENSITY_KERNEL_ENCODE_STATE returnState;

    switch (state->shift) {
        case 0:
            out->pointer -= sizeof(density_lion_signature);
            if ((returnState = density_lion_encode_prepare_new_block(out, state)))
                return returnState;
            break;
        default:
            break;
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
};

DENSITY_FORCE_INLINE void density_lion_encode_push_to_signature(density_memory_location *restrict out, density_lion_encode_state *restrict state, const uint64_t content, const uint_fast8_t bits) {
    density_lion_encode_push_to_proximity_signature(state, content, bits);

    if (density_unlikely(state->shift >= 64)) {
        *state->signature = state->proximitySignature;

        const uint_fast8_t remainder = (uint_fast8_t) (state->shift & 0x3F);
        density_lion_encode_prepare_new_signature(out, state);
        if (remainder)
            density_lion_encode_push_to_proximity_signature(state, content >> (bits - remainder), remainder);
    }
}

DENSITY_FORCE_INLINE density_lion_entropy_code density_lion_encode_get_form_code(density_lion_encode_state *restrict state, const DENSITY_LION_FORM form) {
    density_lion_form_node *form_found = state->formData.formsIndex[form];
    form_found->usage++;

    density_lion_form_node *previous_form = form_found->previousForm;
    if (form_found->previousForm) {
        const uint8_t rank = form_found->rank;

        if (density_unlikely(previous_form->usage < form_found->usage)) {    // Relative stability is assumed
            DENSITY_LION_FORM previous_form_value = previous_form->form;
            uint32_t previous_form_usage = previous_form->usage;

            previous_form->form = form;
            previous_form->usage = form_found->usage;

            form_found->form = previous_form_value;
            form_found->usage = previous_form_usage;

            state->formData.formsIndex[form] = previous_form;
            state->formData.formsIndex[previous_form_value] = form_found;
        }

        return density_lion_form_entropy_codes[rank];
    } else
        return density_lion_form_entropy_codes[0];
}

DENSITY_FORCE_INLINE void density_lion_encode_update_unigram_model(density_lion_encode_state *restrict state, const uint8_t unigram, density_lion_dictionary_unigram_node *unigram_found) {
    if (density_likely(unigram_found)) {
        const uint8_t rank = unigram_found->rank;
        if (rank) {
            density_lion_dictionary_unigram_node *previous_unigram = unigram_found->previousUnigram;
            uint8_t previous_unigram_value = previous_unigram->unigram;
            previous_unigram->unigram = unigram;
            unigram_found->unigram = previous_unigram_value;
            state->dictionary.unigramsIndex[unigram] = previous_unigram;
            state->dictionary.unigramsIndex[previous_unigram_value] = unigram_found;
        }
    } else {
        density_lion_dictionary_unigram_node *new_unigram = &state->dictionary.unigramsPool[state->dictionary.nextAvailableUnigram];
        new_unigram->unigram = unigram;
        new_unigram->previousUnigram = state->dictionary.lastUnigramNode;
        new_unigram->rank = state->dictionary.nextAvailableUnigram++;
        state->dictionary.unigramsIndex[unigram] = new_unigram;
        state->dictionary.lastUnigramNode = new_unigram;
    }
}

DENSITY_FORCE_INLINE void density_lion_encode_process_bigram(density_memory_location *restrict out, density_lion_encode_state *restrict state, const uint16_t bigram) {
    const uint8_t unigram_a = (uint8_t) (bigram & 0xFF);
    const uint8_t unigram_b = (uint8_t) (bigram >> 8);

    density_lion_dictionary_unigram_node *unigram_found_a = state->dictionary.unigramsIndex[unigram_a];

    if (density_likely(unigram_found_a)) {
        density_lion_dictionary_unigram_node *unigram_found_b = state->dictionary.unigramsIndex[unigram_b];

        if (density_likely(unigram_found_b)) {
            const uint8_t rank_a = unigram_found_a->rank;
            const bool qualify_rank_a = (rank_a < 64);

            if (density_likely(qualify_rank_a)) {
                const uint8_t rank_b = unigram_found_b->rank;
                const bool qualify_rank_b = (rank_b < 64);

                if (density_likely(qualify_rank_b)) {
                    density_lion_encode_push_to_signature(out, state, (uint64_t) (((rank_a >> 1) & 0xFE) | DENSITY_LION_BIGRAM_SECONDARY_SIGNATURE_FLAG_ENCODED), 5);
                    *out->pointer = (rank_b | (rank_a << 6));
                    out->pointer += sizeof(uint8_t);

                    return; // Model seems alright, exit
                }
            }
        }
    }

    density_lion_encode_push_to_signature(out, state, DENSITY_LION_BIGRAM_SECONDARY_SIGNATURE_FLAG_PLAIN, 1);
    *(uint16_t *) out->pointer = DENSITY_LITTLE_ENDIAN_16(bigram);
    out->pointer += sizeof(uint16_t);

    density_lion_encode_update_unigram_model(state, unigram_a, unigram_found_a);
    density_lion_encode_update_unigram_model(state, unigram_b, state->dictionary.unigramsIndex[unigram_b]);
}

DENSITY_FORCE_INLINE void density_lion_encode_kernel(density_memory_location *restrict out, uint32_t *restrict hash, const uint32_t chunk, density_lion_encode_state *restrict state) {
    DENSITY_LION_HASH_ALGORITHM(*hash, DENSITY_LITTLE_ENDIAN_32(chunk));
    uint32_t *predictedChunk = &(state->dictionary.predictions[state->lastHash].next_chunk_prediction);

    if (*predictedChunk ^ chunk) {
        density_lion_dictionary_chunk_entry *found = &state->dictionary.chunks[*hash];
        uint32_t *found_a = &found->chunk_a;
        if (*found_a ^ chunk) {
            uint32_t *found_b = &found->chunk_b;
            if (*found_b ^ chunk) {
                const density_lion_entropy_code code = density_lion_encode_get_form_code(state, DENSITY_LION_FORM_SECONDARY_ACCESS);
                density_lion_encode_push_to_signature(out, state, code.value, code.bitLength);

                const uint32_t chunk_rs8 = chunk >> 8;
                const uint32_t chunk_rs16 = chunk >> 16;

                const uint16_t bigram_p = (uint16_t) ((state->lastChunk >> 24) | ((chunk & 0xFF) << 8));
                const uint16_t bigram_a = (uint16_t) (chunk & 0xFFFF);
                const uint16_t bigram_b = (uint16_t) (chunk_rs8 & 0xFFFF);
                const uint16_t bigram_c = (uint16_t) (chunk_rs16 & 0xFFFF);

                const uint8_t hash_p = DENSITY_LION_BIGRAM_HASH_ALGORITHM(bigram_p);
                const uint8_t hash_a = DENSITY_LION_BIGRAM_HASH_ALGORITHM(bigram_a);
                const uint8_t hash_b = DENSITY_LION_BIGRAM_HASH_ALGORITHM(bigram_b);
                const uint8_t hash_c = DENSITY_LION_BIGRAM_HASH_ALGORITHM(bigram_c);

                density_lion_dictionary_bigram_entry *bigram_entry_a = &state->dictionary.bigrams[hash_a];
                density_lion_dictionary_bigram_entry *bigram_entry_c = &state->dictionary.bigrams[hash_c];

                if (bigram_entry_a->bigram == bigram_a) {
                    density_lion_encode_push_to_signature(out, state, DENSITY_LION_BIGRAM_PRIMARY_SIGNATURE_FLAG_DICTIONARY, 1);

                    *(out->pointer) = hash_a;
                    out->pointer++;
                } else {
                    density_lion_encode_push_to_signature(out, state, DENSITY_LION_BIGRAM_PRIMARY_SIGNATURE_FLAG_SECONDARY_ACCESS, 1);

                    density_lion_encode_process_bigram(out, state, bigram_a);
                }
                if (bigram_entry_c->bigram == bigram_c) {
                    density_lion_encode_push_to_signature(out, state, DENSITY_LION_BIGRAM_PRIMARY_SIGNATURE_FLAG_DICTIONARY, 1);

                    *(out->pointer) = hash_c;
                    out->pointer++;
                } else {
                    density_lion_encode_push_to_signature(out, state, DENSITY_LION_BIGRAM_PRIMARY_SIGNATURE_FLAG_SECONDARY_ACCESS, 1);

                    density_lion_encode_process_bigram(out, state, bigram_c);
                }

                state->dictionary.bigrams[hash_p].bigram = bigram_p;
                bigram_entry_a->bigram = bigram_a;
                state->dictionary.bigrams[hash_b].bigram = bigram_b;
                bigram_entry_c->bigram = bigram_c;
            } else {
                const density_lion_entropy_code code = density_lion_encode_get_form_code(state, DENSITY_LION_FORM_CHUNK_DICTIONARY_B);
                density_lion_encode_push_to_signature(out, state, code.value, code.bitLength);

                *(uint16_t *) (out->pointer) = DENSITY_LITTLE_ENDIAN_16(*hash);
                out->pointer += sizeof(uint16_t);
            }
            *found_b = *found_a;
            *found_a = chunk;
        } else {
            const density_lion_entropy_code code = density_lion_encode_get_form_code(state, DENSITY_LION_FORM_CHUNK_DICTIONARY_A);
            density_lion_encode_push_to_signature(out, state, code.value, code.bitLength);

            *(uint16_t *) (out->pointer) = DENSITY_LITTLE_ENDIAN_16(*hash);
            out->pointer += sizeof(uint16_t);
        }
        *predictedChunk = chunk;
    } else {
        const density_lion_entropy_code code = density_lion_encode_get_form_code(state, DENSITY_LION_FORM_CHUNK_PREDICTIONS);
        density_lion_encode_push_to_signature(out, state, code.value, code.bitLength);
    }

    state->lastHash = *hash;
    state->lastChunk = chunk;
}

DENSITY_FORCE_INLINE void density_lion_encode_process_chunk(uint64_t *restrict chunk, density_memory_location *restrict in, density_memory_location *restrict out, uint32_t *restrict hash, density_lion_encode_state *restrict state) {
    *chunk = *(uint64_t *) (in->pointer);

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    density_lion_encode_kernel(out, hash, (uint32_t) (*chunk & 0xFFFFFFFF), state);
#endif
    density_lion_encode_kernel(out, hash, (uint32_t) (*chunk >> 32), state);
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    density_lion_encode_kernel(out, hash, (uint32_t) (*chunk & 0xFFFFFFFF), state);
#endif

    in->pointer += sizeof(uint64_t);
}

DENSITY_FORCE_INLINE void density_lion_encode_process_span(uint64_t *restrict chunk, density_memory_location *restrict in, density_memory_location *restrict out, uint32_t *restrict hash, density_lion_encode_state *restrict state) {
    density_lion_encode_process_chunk(chunk, in, out, hash, state);
    density_lion_encode_process_chunk(chunk, in, out, hash, state);
}

DENSITY_FORCE_INLINE void density_lion_encode_process_unit(uint64_t *restrict chunk, density_memory_location *restrict in, density_memory_location *restrict out, uint32_t *restrict hash, density_lion_encode_state *restrict state) {
    density_lion_encode_process_span(chunk, in, out, hash, state);
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE density_lion_encode_init(density_lion_encode_state *state) {
    state->signaturesCount = 0;
    state->efficiencyChecked = 0;
    density_lion_dictionary_reset(&state->dictionary);

#if DENSITY_ENABLE_PARALLELIZABLE_DECOMPRESSIBLE_OUTPUT == DENSITY_YES
    state->resetCycle = DENSITY_DICTIONARY_PREFERRED_RESET_CYCLE - 1;
#endif

    density_lion_form_model_init(&state->formData);

    state->lastHash = 0;
    state->lastChunk = 0;

    return exitProcess(state, DENSITY_LION_ENCODE_PROCESS_PREPARE_NEW_BLOCK, DENSITY_KERNEL_ENCODE_STATE_READY);
}

#define DENSITY_LION_ENCODE_CONTINUE
#define GENERIC_NAME(name) name ## continue

#include "kernel_lion_generic_encode.h"

#undef GENERIC_NAME
#undef DENSITY_LION_ENCODE_CONTINUE

#define GENERIC_NAME(name) name ## finish

#include "kernel_lion_generic_encode.h"

#undef GENERIC_NAME