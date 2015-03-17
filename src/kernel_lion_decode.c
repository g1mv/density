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
 * 8/03/15 11:59
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

#include "kernel_lion_decode.h"
#include "kernel_lion_dictionary.h"
#include "kernel_lion.h"
#include "memory_location.h"
#include "kernel_lion_unigram_model.h"
#include "kernel_lion_form_model.h"

DENSITY_FORCE_INLINE DENSITY_KERNEL_DECODE_STATE exitProcess(density_lion_decode_state *state, DENSITY_LION_DECODE_PROCESS process, DENSITY_KERNEL_DECODE_STATE kernelDecodeState) {
    state->process = process;
    return kernelDecodeState;
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_DECODE_STATE density_lion_decode_check_block_state(density_lion_decode_state *restrict state) {
    if ((state->chunksCount >= DENSITY_LION_PREFERRED_EFFICIENCY_CHECK_CHUNKS) && (!state->efficiencyChecked)) {
        state->efficiencyChecked = true;
        return DENSITY_KERNEL_DECODE_STATE_INFO_EFFICIENCY_CHECK;
    } else if (state->chunksCount >= DENSITY_LION_PREFERRED_BLOCK_CHUNKS) {
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

        return DENSITY_KERNEL_DECODE_STATE_INFO_NEW_BLOCK;
    }

    return DENSITY_KERNEL_DECODE_STATE_READY;
}

DENSITY_FORCE_INLINE void density_lion_decode_read_signature_from_memory(density_memory_location *restrict in, density_lion_decode_state *restrict state) {
    state->signature = DENSITY_LITTLE_ENDIAN_64(*(density_lion_signature *) (in->pointer));
    in->pointer += sizeof(density_lion_signature);
}

DENSITY_FORCE_INLINE uint8_t density_lion_decode_read_4bits_from_signature(density_memory_location *restrict in, density_lion_decode_state *restrict state) {
    uint_fast8_t result;
    const uint_fast32_t projected_shift = state->shift + 4;

    if (density_likely(state->shift)) {
        if (density_unlikely(projected_shift >= density_bitsizeof(density_lion_signature))) {
            result = (uint8_t) (state->signature >> state->shift);  // Get the remaining bits from the current signature
            uint_fast8_t overflowBits = (uint_fast8_t) (projected_shift & (density_bitsizeof(density_lion_signature) - 1));

            if (overflowBits) {
                density_lion_decode_read_signature_from_memory(in, state);

                result |= (state->signature & density_lion_decode_bitmasks[overflowBits]) << (density_bitsizeof(density_lion_signature) - state->shift);   // Add bits from the new signature
            }

            state->shift = overflowBits;

            return result;
        } else {
            result = (uint8_t) ((state->signature >> state->shift) & 0xF);   // No overflow, we return the bits requested from the current signature
            state->shift = projected_shift;

            return result;
        }
    } else {
        density_lion_decode_read_signature_from_memory(in, state);

        result = (uint8_t) ((state->signature >> state->shift) & 0xF);
        state->shift = projected_shift;

        return result;
    }
}

DENSITY_FORCE_INLINE bool density_lion_decode_read_1bit_from_signature(density_memory_location *restrict in, density_lion_decode_state *restrict state) {
    if (density_likely(state->shift)) {
        bool result = (bool) ((state->signature >> state->shift) & 0x1);

        switch (state->shift) {
            case (density_bitsizeof(density_lion_signature) - 1):
                state->shift = 0;
                break;
            default:
                state->shift++;
                break;
        }

        return result;
    } else {
        density_lion_decode_read_signature_from_memory(in, state);

        return (bool) ((state->signature >> (state->shift++)) & 0x1);
    }
}

DENSITY_FORCE_INLINE void density_lion_decode_read_chunk_hash(uint32_t *restrict hash, density_memory_location *restrict in) {
    *hash = *(uint16_t *) (in->pointer);
    in->pointer += sizeof(uint16_t);
}

DENSITY_FORCE_INLINE void density_lion_decode_process_chunk(uint32_t *restrict hash, const uint32_t *restrict chunk, density_memory_location *restrict out, density_lion_decode_state *restrict state) {
    density_lion_dictionary_chunk_entry *entry;

    DENSITY_LION_HASH_ALGORITHM(*hash, DENSITY_LITTLE_ENDIAN_32(*chunk));
    entry = &state->dictionary.chunks[*hash];
    entry->chunk_b = entry->chunk_a;
    entry->chunk_a = *chunk;

    (&state->dictionary.predictions[state->lastHash])->next_chunk_prediction = *chunk;
}

DENSITY_FORCE_INLINE void density_lion_decode_predicted_chunk(uint32_t *restrict hash, density_memory_location *restrict out, density_lion_decode_state *restrict state) {
    uint32_t chunk = (&state->dictionary.predictions[state->lastHash])->next_chunk_prediction;
    DENSITY_LION_HASH_ALGORITHM(*hash, DENSITY_LITTLE_ENDIAN_32(chunk));

    *(uint32_t *) (out->pointer) = chunk;
    out->pointer += sizeof(uint32_t);

    state->lastUnigram = (uint8_t) (chunk >> 24);
}

DENSITY_FORCE_INLINE void density_lion_decode_write_chunk(const uint32_t *restrict chunk, density_memory_location *restrict out, density_lion_decode_state *restrict state) {
    *(uint32_t *) (out->pointer) = *chunk;
    out->pointer += sizeof(uint32_t);

    (&state->dictionary.predictions[state->lastHash])->next_chunk_prediction = *chunk;
    state->lastUnigram = (uint8_t) (*chunk >> 24);
}

DENSITY_FORCE_INLINE void density_lion_decode_compressed_chunk_a(const uint32_t *restrict hash, density_memory_location *restrict out, density_lion_decode_state *restrict state) {
    uint32_t chunk = (&state->dictionary.chunks[DENSITY_LITTLE_ENDIAN_16(*hash)])->chunk_a;

    density_lion_decode_write_chunk(&chunk, out, state);
}

DENSITY_FORCE_INLINE void density_lion_decode_compressed_chunk_b(const uint32_t *restrict hash, density_memory_location *restrict out, density_lion_decode_state *restrict state) {
    density_lion_dictionary_chunk_entry *entry = &state->dictionary.chunks[DENSITY_LITTLE_ENDIAN_16(*hash)];
    uint32_t swapped_chunk = entry->chunk_b;

    entry->chunk_b = entry->chunk_a;
    entry->chunk_a = swapped_chunk;

    density_lion_decode_write_chunk(&swapped_chunk, out, state);
}

DENSITY_FORCE_INLINE uint8_t density_lion_decode_bigram(density_memory_location *restrict in, density_memory_location *restrict out, density_lion_decode_state *restrict state, const uint8_t previous_unigram, uint32_t *restrict chunk, const uint8_t shift) {
    uint16_t bigram;
    uint8_t unigram_a;
    uint8_t unigram_b;

    if (density_lion_decode_read_1bit_from_signature(in, state)) {  // DENSITY_LION_BIGRAM_PRIMARY_SIGNATURE_FLAG_SECONDARY_ACCESS
        if (density_lion_decode_read_1bit_from_signature(in, state)) {  // DENSITY_LION_BIGRAM_SECONDARY_SIGNATURE_FLAG_PLAIN
            bigram = *(uint16_t *) in->pointer;
            in->pointer += sizeof(uint16_t);

            unigram_a = (uint8_t) (bigram & 0xFF);
            unigram_b = (uint8_t) (bigram >> 8);

            density_lion_unigram_model_update(&state->unigramData, unigram_a, state->unigramData.unigramsIndex[unigram_a]);
            density_lion_unigram_model_update(&state->unigramData, unigram_b, state->unigramData.unigramsIndex[unigram_b]);
        } else {    // DENSITY_LION_BIGRAM_SECONDARY_SIGNATURE_FLAG_ENCODED:
            const uint8_t partialBits = density_lion_decode_read_4bits_from_signature(in, state);

            uint8_t byte = *in->pointer;
            in->pointer++;

            const uint_fast8_t rank_a = (uint_fast8_t) (((byte >> 6) & BINARY_TO_UINT(11)) | (partialBits << 2));
            const uint_fast8_t rank_b = (uint_fast8_t) (byte & BINARY_TO_UINT(111111));

            density_lion_unigram_node *found_a = &state->unigramData.unigramsPool[rank_a];
            density_lion_unigram_node *found_b = &state->unigramData.unigramsPool[rank_b];

            unigram_a = found_a->unigram;
            unigram_b = found_b->unigram;

            bigram = ((unigram_b << 8) | unigram_a);
            // Unigram model seems alright, exit
        }

        // Update dictionary values
        const uint8_t hash = DENSITY_LION_BIGRAM_HASH_ALGORITHM(bigram);
        state->dictionary.bigrams[hash].bigram = bigram;

    } else {  // DENSITY_LION_BIGRAM_PRIMARY_SIGNATURE_FLAG_DICTIONARY:
        uint8_t bigramHash = *in->pointer;
        in->pointer++;

        bigram = (&state->dictionary.bigrams[bigramHash])->bigram;
        unigram_a = (uint8_t) (bigram & 0xFF);
        unigram_b = (uint8_t) (bigram >> 8);
    }

    // Update remaining dictionary values
    const uint16_t bigram_p = ((unigram_a << 8) | previous_unigram);
    const uint8_t hash_p = DENSITY_LION_BIGRAM_HASH_ALGORITHM(bigram_p);
    state->dictionary.bigrams[hash_p].bigram = bigram_p;

    // Write bigram to output
    *(uint16_t *) out->pointer = bigram;
    out->pointer += sizeof(uint16_t);

    *chunk |= (bigram << shift);

    return unigram_b;
}

DENSITY_FORCE_INLINE void density_lion_decode_chunk(density_memory_location *restrict in, density_memory_location *restrict out, density_lion_decode_state *restrict state, const DENSITY_LION_FORM form) {
    uint32_t hash = 0;
    uint32_t chunk = 0;

    switch (form) {
        case DENSITY_LION_FORM_CHUNK_PREDICTIONS:
            density_lion_decode_predicted_chunk(&hash, out, state);
            break;
        case DENSITY_LION_FORM_CHUNK_DICTIONARY_A:
            density_lion_decode_read_chunk_hash(&hash, in);
            density_lion_decode_compressed_chunk_a(&hash, out, state);
            break;
        case DENSITY_LION_FORM_CHUNK_DICTIONARY_B:
            density_lion_decode_read_chunk_hash(&hash, in);
            density_lion_decode_compressed_chunk_b(&hash, out, state);
            break;
        case DENSITY_LION_FORM_SECONDARY_ACCESS:
            state->lastUnigram = density_lion_decode_bigram(in, out, state, density_lion_decode_bigram(in, out, state, state->lastUnigram, &chunk, 0), &chunk, 0x10);
            density_lion_decode_process_chunk(&hash, &chunk, out, state);
            break;
    }

    state->lastHash = hash;
}

DENSITY_FORCE_INLINE const DENSITY_LION_FORM density_lion_decode_read_form(density_memory_location *restrict in, density_lion_decode_state *restrict state) {
    density_lion_form_node *form;

    const bool first_bit = density_lion_decode_read_1bit_from_signature(in, state);
    if (density_unlikely(first_bit)) {
        const bool second_bit = density_lion_decode_read_1bit_from_signature(in, state);
        if (density_unlikely(second_bit)) {
            const bool third_bit = density_lion_decode_read_1bit_from_signature(in, state);
            if (density_unlikely(third_bit))
                form = &state->formData.formsPool[3];
            else
                form = &state->formData.formsPool[2];
        } else
            form = &state->formData.formsPool[1];
    } else
        form = &state->formData.formsPool[0];

    const DENSITY_LION_FORM formValue = form->form;
    form->usage++;

    if (density_unlikely(form->previousForm))
        density_lion_form_model_update(&state->formData, form, form->previousForm);

    return formValue;
}

DENSITY_FORCE_INLINE void density_lion_decode_process_span(density_memory_location *restrict in, density_memory_location *restrict out, density_lion_decode_state *restrict state) {
    density_lion_decode_chunk(in, out, state, density_lion_decode_read_form(in, state));
    density_lion_decode_chunk(in, out, state, density_lion_decode_read_form(in, state));
    density_lion_decode_chunk(in, out, state, density_lion_decode_read_form(in, state));
    density_lion_decode_chunk(in, out, state, density_lion_decode_read_form(in, state));
}

DENSITY_FORCE_INLINE void density_lion_decode_process_unit(density_memory_location *restrict in, density_memory_location *restrict out, density_lion_decode_state *restrict state) {
    density_lion_decode_process_span(in, out, state);

    state->chunksCount += DENSITY_LION_DECODE_CHUNKS_PER_PROCESS_UNIT;
}

DENSITY_FORCE_INLINE bool density_lion_decode_unit_step_by_step(density_memory_location *restrict readMemoryLocation, density_memory_teleport *restrict in, density_memory_location *restrict out, density_lion_decode_state *restrict state) {
    DENSITY_LION_FORM form = density_lion_decode_read_form(readMemoryLocation, state);
    switch (form) {
        case DENSITY_LION_FORM_CHUNK_DICTIONARY_A:  // Potential end marker, we need 2 bytes for a chunk dictionary hash, if remaining bytes < 2 + 2 bytes then this form is the last one
            if (density_unlikely(density_memory_teleport_available_bytes_reserved(in, state->endDataOverhead) < sizeof(uint32_t)))
                return false;
            break;
        default:
            break;
    }
    density_lion_decode_chunk(readMemoryLocation, out, state, form);
    return true;
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_DECODE_STATE density_lion_decode_init(density_lion_decode_state *state, const density_main_header_parameters parameters, const uint_fast32_t endDataOverhead) {
    state->chunksCount = 0;
    state->efficiencyChecked = false;
    state->readSignature = true;
    density_lion_dictionary_reset(&state->dictionary);

    state->parameters = parameters;
    density_byte resetDictionaryCycleShift = state->parameters.as_bytes[0];
    if (resetDictionaryCycleShift)
        state->resetCycle = (uint_fast64_t) (1 << resetDictionaryCycleShift) - 1;

    state->endDataOverhead = endDataOverhead;

    density_lion_form_model_init(&state->formData);
    density_lion_unigram_model_init(&state->unigramData);

    state->lastHash = 0;
    state->lastUnigram = 0;

    return exitProcess(state, DENSITY_LION_DECODE_PROCESS_CHECK_BLOCK_STATE, DENSITY_KERNEL_DECODE_STATE_READY);
}

#define DENSITY_LION_DECODE_CONTINUE
#define GENERIC_NAME(name) name ## continue

#include "kernel_lion_generic_decode.h"

#undef GENERIC_NAME
#undef DENSITY_LION_DECODE_CONTINUE

#define GENERIC_NAME(name) name ## finish

#include "kernel_lion_generic_decode.h"

#undef GENERIC_NAME