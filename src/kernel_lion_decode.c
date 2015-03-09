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

DENSITY_FORCE_INLINE DENSITY_KERNEL_DECODE_STATE exitProcess(density_lion_decode_state *state, DENSITY_LION_DECODE_PROCESS process, DENSITY_KERNEL_DECODE_STATE kernelDecodeState) {
    state->process = process;
    return kernelDecodeState;
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_DECODE_STATE density_lion_decode_check_state(density_memory_location *restrict out, density_lion_decode_state *restrict state) {
    if (out->available_bytes < (DENSITY_LION_DECODE_MAXIMUM_BODY_SIZE_PER_SIGNATURE << DENSITY_LION_DECODE_ITERATIONS_SHIFT))
        return DENSITY_KERNEL_DECODE_STATE_STALL_ON_OUTPUT;

    switch (state->signaturesCount) {
        case DENSITY_LION_PREFERRED_EFFICIENCY_CHECK_SIGNATURES:
            if (state->efficiencyChecked ^ 0x1) {
                state->efficiencyChecked = 1;
                return DENSITY_KERNEL_DECODE_STATE_INFO_EFFICIENCY_CHECK;
            }
            break;
        case DENSITY_LION_PREFERRED_BLOCK_SIGNATURES:
            state->signaturesCount = 0;
            state->efficiencyChecked = 0;

            if (state->resetCycle)
                state->resetCycle--;
            else {
                density_byte resetDictionaryCycleShift = state->parameters.as_bytes[0];
                if (resetDictionaryCycleShift) {
                    density_lion_dictionary_reset(&state->dictionary);
                    state->resetCycle = (uint_fast64_t) (1 << resetDictionaryCycleShift) - 1;
                }
            }

            return DENSITY_KERNEL_DECODE_STATE_INFO_NEW_BLOCK;
        default:
            break;
    }
    return DENSITY_KERNEL_DECODE_STATE_READY;
}

DENSITY_FORCE_INLINE void density_lion_decode_read_signature_from_memory(density_memory_location *restrict in, density_lion_decode_state *restrict state) {
    state->signature = DENSITY_LITTLE_ENDIAN_64(*(density_lion_signature *) (in->pointer));
    in->pointer += sizeof(density_lion_signature);
    state->signaturesCount++;
}

/*DENSITY_FORCE_INLINE uint64_t density_lion_decode_read_bits_from_signature(density_memory_location *restrict in, density_lion_decode_state *restrict state, const uint_fast8_t bits) {
    if (density_unlikely(!state->shift)) {
        density_lion_decode_read_signature_from_memory(in, state);

        return (state->signature >> (state->shift += bits)) & density_lion_decode_bitmasks[bits];
    } else {
        const uint64_t projected_shift = state->shift + bits;

        if (density_unlikely(projected_shift >= density_bitsizeof(density_lion_signature))) {
            uint64_t mix = (state->signature >> state->shift);  // Get the remaining bits from the current signature
            uint_fast8_t overflowBits = (uint_fast8_t) (projected_shift & 0x3F);

            density_lion_decode_read_signature_from_memory(in, state);

            mix |= (state->signature & density_lion_decode_bitmasks[overflowBits]) << (density_bitsizeof(density_lion_signature) - state->shift);   // Add bits from the new signature
            state->shift = overflowBits;

            return mix;
        } else
            return (state->signature >> (state->shift += bits)) & density_lion_decode_bitmasks[bits];   // No overflow, we return the bits requested from the current signature
    }
}*/

DENSITY_FORCE_INLINE uint8_t density_lion_decode_read_4bits_from_signature(density_memory_location *restrict in, density_lion_decode_state *restrict state) {
    uint_fast8_t result;
    const uint_fast8_t projected_shift = state->shift + (uint8_t) 4;

    if (density_likely(state->shift)) {
        if (density_unlikely(projected_shift >= density_bitsizeof(density_lion_signature))) {
            result = (uint8_t) (state->signature >> state->shift);  // Get the remaining bits from the current signature
            uint_fast8_t overflowBits = (uint_fast8_t) (projected_shift & (density_bitsizeof(density_lion_signature) - 1));

            density_lion_decode_read_signature_from_memory(in, state);

            result |= (state->signature & density_lion_decode_bitmasks[overflowBits]) << (density_bitsizeof(density_lion_signature) - state->shift);   // Add bits from the new signature
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
        bool result = (bool) (state->signature >> state->shift);

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

DENSITY_FORCE_INLINE void density_lion_decode_read_byte(uint8_t *restrict hash, density_memory_location *restrict in) {
    *hash = *in->pointer;
    in->pointer++;
}

DENSITY_FORCE_INLINE void density_lion_decode_read_chunk_hash(uint16_t *restrict hash, density_memory_location *restrict in) {
    *hash = *(uint16_t *) (in->pointer);
    in->pointer += sizeof(uint16_t);
}

/*DENSITY_FORCE_INLINE void density_lion_decode_read_uncompressed_chunk(uint32_t *restrict chunk, density_memory_location *restrict in) {
    *chunk = *(uint32_t *) (in->pointer);
    in->pointer += sizeof(uint32_t);
}*/

DENSITY_FORCE_INLINE void density_lion_decode_predicted_chunk(uint16_t *restrict hash, density_memory_location *restrict out, density_lion_decode_state *restrict state) {
    uint32_t chunk = (&state->dictionary.predictions[state->lastHash])->next_chunk_prediction;
    DENSITY_LION_HASH_ALGORITHM(*hash, DENSITY_LITTLE_ENDIAN_32(chunk));

    *(uint32_t *) (out->pointer) = chunk;
    out->pointer += sizeof(uint32_t);
}

DENSITY_FORCE_INLINE void density_lion_decode_write_chunk(const uint32_t *restrict chunk, density_memory_location *restrict out, density_lion_decode_state *restrict state) {
    *(uint32_t *) (out->pointer) = *chunk;
    out->pointer += sizeof(uint32_t);

    (&state->dictionary.predictions[state->lastHash])->next_chunk_prediction = *chunk;
    state->lastChunk = *chunk;
}

DENSITY_FORCE_INLINE void density_lion_decode_compressed_bigram(const uint8_t *restrict hash, density_memory_location *restrict out, density_lion_decode_state *restrict state) {
    *(uint16_t *) (out->pointer) = (&state->dictionary.bigrams[*hash])->bigram;
    out->pointer += sizeof(uint16_t);
}

DENSITY_FORCE_INLINE void density_lion_decode_compressed_chunk_a(const uint16_t *restrict hash, density_memory_location *restrict out, density_lion_decode_state *restrict state) {
    uint32_t chunk = (&state->dictionary.chunks[DENSITY_LITTLE_ENDIAN_16(*hash)])->chunk_a;

    density_lion_decode_write_chunk(&chunk, out, state);
}

DENSITY_FORCE_INLINE void density_lion_decode_compressed_chunk_b(const uint16_t *restrict hash, density_memory_location *restrict out, density_lion_decode_state *restrict state) {
    density_lion_dictionary_chunk_entry *entry = &state->dictionary.chunks[DENSITY_LITTLE_ENDIAN_16(*hash)];
    uint32_t swapped_chunk = entry->chunk_b;

    entry->chunk_b = entry->chunk_a;
    entry->chunk_a = swapped_chunk;

    density_lion_decode_write_chunk(&swapped_chunk, out, state);
}

/*DENSITY_FORCE_INLINE void density_lion_decode_uncompressed_chunk(uint32_t *restrict hash, const uint32_t *restrict chunk, density_memory_location *restrict out, density_lion_decode_state *restrict state) {
    density_lion_dictionary_chunk_entry *entry;

    DENSITY_LION_HASH_ALGORITHM(*hash, DENSITY_LITTLE_ENDIAN_32(*chunk));
    entry = &state->dictionary.chunks[*hash];
    entry->chunk_b = entry->chunk_a;
    entry->chunk_a = *chunk;

    density_lion_decode_write_decompressed_chunk(chunk, out, state);
}*/

DENSITY_FORCE_INLINE void density_lion_decode_bigram(density_memory_location *restrict in, density_memory_location *restrict out, density_lion_decode_state *restrict state) {
    /*const uint32_t chunk_rs8 = chunk >> 8;
    const uint32_t chunk_rs16 = chunk >> 16;

    const uint16_t bigram_p = (uint16_t) ((state->lastChunk >> 24) | ((chunk & 0xFF) << 8));
    const uint16_t bigram_a = (uint16_t) (chunk & 0xFFFF);
    const uint16_t bigram_b = (uint16_t) (chunk_rs8 & 0xFFFF);
    const uint16_t bigram_c = (uint16_t) (chunk_rs16 & 0xFFFF);

    const uint8_t hash_p = DENSITY_LION_BIGRAM_HASH_ALGORITHM(bigram_p);
    const uint8_t hash_a = DENSITY_LION_BIGRAM_HASH_ALGORITHM(bigram_a);
    const uint8_t hash_b = DENSITY_LION_BIGRAM_HASH_ALGORITHM(bigram_b);
    const uint8_t hash_c = DENSITY_LION_BIGRAM_HASH_ALGORITHM(bigram_c);*/

    uint8_t bigramHash;
    uint8_t rank_a;
    uint8_t rank_b;
    uint8_t byte;

    if (density_lion_decode_read_1bit_from_signature(in, state)) {  // DENSITY_LION_BIGRAM_PRIMARY_SIGNATURE_FLAG_SECONDARY_ACCESS
        if (density_lion_decode_read_1bit_from_signature(in, state)) {  // DENSITY_LION_BIGRAM_SECONDARY_SIGNATURE_FLAG_PLAIN
            *(uint16_t *) out->pointer = *(uint16_t *) in->pointer;
            in->pointer += sizeof(uint16_t);
        } else {    // DENSITY_LION_BIGRAM_SECONDARY_SIGNATURE_FLAG_ENCODED:
            density_lion_decode_read_byte(&byte, in);
            rank_a = (uint8_t) (((byte >> 2) & BINARY_TO_UINT(110000)) | density_lion_decode_read_4bits_from_signature(in, state));
            rank_b = (uint8_t) (byte & BINARY_TO_UINT(111111));
            // write letters from rank;
            out->pointer += sizeof(uint16_t);
        }
    } else {  // DENSITY_LION_BIGRAM_PRIMARY_SIGNATURE_FLAG_DICTIONARY:
        density_lion_decode_read_byte(&bigramHash, in);
        density_lion_decode_compressed_bigram(&bigramHash, out, state);
    }
}

DENSITY_FORCE_INLINE void density_lion_decode_kernel(density_memory_location *restrict in, density_memory_location *restrict out, density_lion_decode_state *restrict state, const DENSITY_LION_FORM form) {
    uint16_t hash = 0;

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
            density_lion_decode_bigram(in, out, state);
            density_lion_decode_bigram(in, out, state);
            break;
    }

    state->lastHash = hash;
    state->lastChunk = *(uint32_t *) (out->pointer - sizeof(uint32_t));
}

// todo to be improved
/*DENSITY_FORCE_INLINE void density_lion_decode_update_form_model(density_lion_decode_state *restrict state, DENSITY_LION_FORM form) {
    density_lion_form_statistics *stats = &state->formStatistics[form];

    const uint8_t rank = stats->rank;

    if (density_unlikely(rank)) {
        density_lion_form_rank *rankCurrent = &state->formRanks[rank];
        density_lion_form_rank *rankUpper = &state->formRanks[rank - 1];
        if (density_unlikely(stats->usage > rankUpper->statistics->usage)) {
            density_lion_form_statistics *replaced = rankUpper->statistics;
            replaced->rank++;
            stats->rank--;
            rankUpper->statistics = stats;
            rankCurrent->statistics = replaced;
        }
    }
    stats->usage++;
}*/

/*DENSITY_FORCE_INLINE const DENSITY_LION_FORM density_lion_decode_read_form(density_memory_location *restrict in, density_lion_decode_state *restrict state) {
    DENSITY_LION_FORM form;

    const bool first_bit = density_lion_decode_read_1bit_from_signature(in, state);
    if (density_unlikely(first_bit)) {
        const bool second_bit = density_lion_decode_read_1bit_from_signature(in, state);
        if (density_unlikely(second_bit)) {
            const bool third_bit = density_lion_decode_read_1bit_from_signature(in, state);
            if (density_unlikely(third_bit))
                form = state->formRanks[3].statistics->form;
            else
                form = state->formRanks[2].statistics->form;
        } else
            form = state->formRanks[1].statistics->form;
    } else
        form = state->formRanks[0].statistics->form;

    density_lion_decode_update_form_model(state, form);

    return form;
}*/

DENSITY_FORCE_INLINE void density_lion_decode_process_data(density_memory_location *restrict in, density_memory_location *restrict out, density_lion_decode_state *restrict state) {
    //while (density_likely(state->shift))
    //    density_lion_decode_kernel(in, out, state, density_lion_decode_read_form(in, state));
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_DECODE_STATE density_lion_decode_init(density_lion_decode_state *state, const density_main_header_parameters parameters, const uint_fast32_t endDataOverhead) {
    state->signaturesCount = 0;
    state->efficiencyChecked = 0;
    density_lion_dictionary_reset(&state->dictionary);

    state->parameters = parameters;
    density_byte resetDictionaryCycleShift = state->parameters.as_bytes[0];
    if (resetDictionaryCycleShift)
        state->resetCycle = (uint_fast64_t) (1 << resetDictionaryCycleShift) - 1;

    state->endDataOverhead = endDataOverhead;

    /*state->formStatistics[DENSITY_LION_FORM_CHUNK_PREDICTIONS].form = DENSITY_LION_FORM_CHUNK_PREDICTIONS;
    state->formStatistics[DENSITY_LION_FORM_CHUNK_PREDICTIONS].usage = 0;
    state->formStatistics[DENSITY_LION_FORM_CHUNK_PREDICTIONS].rank = 3;
    state->formStatistics[DENSITY_LION_FORM_CHUNK_DICTIONARY_A].form = DENSITY_LION_FORM_CHUNK_DICTIONARY_A;
    state->formStatistics[DENSITY_LION_FORM_CHUNK_DICTIONARY_A].usage = 0;
    state->formStatistics[DENSITY_LION_FORM_CHUNK_DICTIONARY_A].rank = 1;
    state->formStatistics[DENSITY_LION_FORM_CHUNK_DICTIONARY_B].form = DENSITY_LION_FORM_CHUNK_DICTIONARY_B;
    state->formStatistics[DENSITY_LION_FORM_CHUNK_DICTIONARY_B].usage = 0;
    state->formStatistics[DENSITY_LION_FORM_CHUNK_DICTIONARY_B].rank = 2;
    state->formStatistics[DENSITY_LION_FORM_SECONDARY_ACCESS].form = DENSITY_LION_FORM_SECONDARY_ACCESS;
    state->formStatistics[DENSITY_LION_FORM_SECONDARY_ACCESS].usage = 0;
    state->formStatistics[DENSITY_LION_FORM_SECONDARY_ACCESS].rank = 0;

    state->formRanks[3].statistics = &state->formStatistics[DENSITY_LION_FORM_CHUNK_PREDICTIONS];
    state->formRanks[1].statistics = &state->formStatistics[DENSITY_LION_FORM_CHUNK_DICTIONARY_A];
    state->formRanks[2].statistics = &state->formStatistics[DENSITY_LION_FORM_CHUNK_DICTIONARY_B];
    state->formRanks[0].statistics = &state->formStatistics[DENSITY_LION_FORM_SECONDARY_ACCESS];*/

    state->lastHash = 0;
    state->lastChunk = 0;

    return exitProcess(state, DENSITY_LION_DECODE_PROCESS_CHECK_SIGNATURE_STATE, DENSITY_KERNEL_DECODE_STATE_READY);
}