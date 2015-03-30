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

const uint8_t density_lion_decode_bitmasks[DENSITY_LION_DECODE_NUMBER_OF_BITMASK_VALUES] = DENSITY_LION_DECODE_BITMASK_VALUES;

DENSITY_FORCE_INLINE DENSITY_KERNEL_DECODE_STATE exitProcess(density_lion_decode_state *state, DENSITY_LION_DECODE_PROCESS process, DENSITY_KERNEL_DECODE_STATE kernelDecodeState) {
    state->process = process;
    return kernelDecodeState;
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_DECODE_STATE density_lion_decode_check_block_state(density_lion_decode_state *restrict state) {
    if (density_unlikely((state->chunksCount >= DENSITY_LION_PREFERRED_EFFICIENCY_CHECK_CHUNKS) && (!state->efficiencyChecked))) {
        state->efficiencyChecked = true;
        return DENSITY_KERNEL_DECODE_STATE_INFO_EFFICIENCY_CHECK;
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

DENSITY_FORCE_INLINE void density_lion_decode_update_predictions_model(density_lion_dictionary_chunk_prediction_entry *restrict predictions, const uint32_t *restrict chunk) {
    *(uint64_t *) ((uint32_t *) predictions + 1) = *(uint64_t *) predictions;
    predictions->next_chunk_a = *chunk;     // Move chunk to the top of the predictions list
}

DENSITY_FORCE_INLINE void density_lion_decode_process_chunk(uint32_t *restrict hash, const uint32_t *restrict chunk, density_lion_decode_state *restrict state) {
    density_lion_dictionary_chunk_entry *entry;

    DENSITY_LION_HASH_ALGORITHM(*hash, DENSITY_LITTLE_ENDIAN_32(*chunk));

    entry = &state->dictionary.chunks[*hash];
    entry->chunk_b = entry->chunk_a;
    entry->chunk_a = *chunk;

    density_lion_dictionary_chunk_prediction_entry *p = &(state->dictionary.predictions[state->lastHash]);
    density_lion_decode_update_predictions_model(p, chunk);

    state->lastChunk = *chunk;
}

DENSITY_FORCE_INLINE void density_lion_decode_predicted_chunk(uint32_t *restrict hash, density_memory_location *restrict out, density_lion_decode_state *restrict state) {
    uint32_t chunk = *(uint32_t *) (&state->dictionary.predictions[state->lastHash]);

    DENSITY_LION_HASH_ALGORITHM(*hash, DENSITY_LITTLE_ENDIAN_32(chunk));

    *(uint32_t *) (out->pointer) = chunk;
    out->pointer += sizeof(uint32_t);

    state->lastChunk = chunk;
}

DENSITY_FORCE_INLINE void density_lion_decode_secondary_predicted_chunk(uint32_t *restrict hash, const bool flag, density_memory_location *restrict out, density_lion_decode_state *restrict state) {
    uint32_t chunk;
    density_lion_dictionary_chunk_prediction_entry *p = &(state->dictionary.predictions[state->lastHash]);

    switch ((DENSITY_LION_PREDICTIONS_SIGNATURE_FLAG) flag) {
        default:    // DENSITY_LION_PREDICTIONS_SIGNATURE_FLAG_A
            chunk = p->next_chunk_b;
            break;

        case DENSITY_LION_PREDICTIONS_SIGNATURE_FLAG_B:
            chunk = p->next_chunk_c;
            break;
    }
    density_lion_decode_update_predictions_model(p, &chunk);

    DENSITY_LION_HASH_ALGORITHM(*hash, DENSITY_LITTLE_ENDIAN_32(chunk));

    *(uint32_t *) (out->pointer) = chunk;
    out->pointer += sizeof(uint32_t);

    state->lastChunk = chunk;
}

DENSITY_FORCE_INLINE void density_lion_decode_write_chunk(const uint32_t *restrict chunk, density_memory_location *restrict out, density_lion_decode_state *restrict state) {
    *(uint32_t *) (out->pointer) = *chunk;
    out->pointer += sizeof(uint32_t);

    density_lion_dictionary_chunk_prediction_entry *p = &(state->dictionary.predictions[state->lastHash]);
    density_lion_decode_update_predictions_model(p, chunk);

    state->lastChunk = *chunk;
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

DENSITY_FORCE_INLINE uint16_t density_lion_decode_bigram(density_memory_location *restrict in, density_memory_location *restrict out, density_lion_decode_state *restrict state/*, const uint8_t previous_unigram, uint32_t *restrict chunk, const uint8_t shift*/) {
    uint16_t bigram;

    if (density_lion_decode_read_1bit_from_signature(in, state)) {  // DENSITY_LION_BIGRAM_SIGNATURE_FLAG_PLAIN
        bigram = *(uint16_t *) in->pointer;
        in->pointer += sizeof(uint16_t);

        // Update dictionary value
        state->dictionary.bigrams[DENSITY_LION_BIGRAM_HASH_ALGORITHM(bigram)].bigram = bigram;
    } else {  // DENSITY_LION_BIGRAM_SIGNATURE_FLAG_DICTIONARY:
        const uint8_t bigramHash = *in->pointer;
        in->pointer += sizeof(uint8_t);

        bigram = (&state->dictionary.bigrams[bigramHash])->bigram;
    }

    // Write bigram to output
    *(uint16_t *) out->pointer = bigram;
    out->pointer += sizeof(uint16_t);

    return bigram;
}

DENSITY_FORCE_INLINE void density_lion_decode_chunk(density_memory_location *restrict in, density_memory_location *restrict out, density_lion_decode_state *restrict state, const DENSITY_LION_FORM form) {
    uint32_t hash = 0;
    uint32_t chunk;

    __builtin_prefetch((uint32_t *) out->pointer + 1, 1, 3);

    switch (form) {
        case DENSITY_LION_FORM_CHUNK_PREDICTIONS:
            density_lion_decode_predicted_chunk(&hash, out, state);
            break;
        case DENSITY_LION_FORM_CHUNK_SECONDARY_PREDICTIONS:
            density_lion_decode_secondary_predicted_chunk(&hash, density_lion_decode_read_1bit_from_signature(in, state), out, state);
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
            chunk = density_lion_decode_bigram(in, out, state) | ((uint32_t) density_lion_decode_bigram(in, out, state) << 16);

            const uint16_t intermediate_bigram_a = (uint16_t) ((chunk << 8) | (state->lastChunk >> 24));
            const uint16_t intermediate_bigram_b = (uint16_t) (chunk >> 8);
            state->dictionary.bigrams[DENSITY_LION_BIGRAM_HASH_ALGORITHM(intermediate_bigram_a)].bigram = intermediate_bigram_a;
            state->dictionary.bigrams[DENSITY_LION_BIGRAM_HASH_ALGORITHM(intermediate_bigram_b)].bigram = intermediate_bigram_b;

            density_lion_decode_process_chunk(&hash, &chunk, state);
            break;
    }

    __builtin_prefetch(&(state->dictionary.predictions[hash]), 1, 3);

    state->lastHash = hash;
}

DENSITY_FORCE_INLINE const DENSITY_LION_FORM density_lion_decode_read_form(density_memory_location *restrict in, density_lion_decode_state *restrict state) {
    density_lion_form_node *form;

    const bool first_bit = density_lion_decode_read_1bit_from_signature(in, state);
    if (density_unlikely(first_bit)) {
        const bool second_bit = density_lion_decode_read_1bit_from_signature(in, state);
        if (density_unlikely(second_bit)) {
            const bool third_bit = density_lion_decode_read_1bit_from_signature(in, state);
            if (density_unlikely(third_bit)) {
                const bool fourth_bit = density_lion_decode_read_1bit_from_signature(in, state);
                if (density_unlikely(fourth_bit))
                    form = &state->formData.formsPool[4];
                else
                    form = &state->formData.formsPool[3];
            } else
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

DENSITY_FORCE_INLINE void density_lion_decode_process_unit(density_memory_location *restrict in, density_memory_location *restrict out, density_lion_decode_state *restrict state) {
    DENSITY_UNROLL_4(density_lion_decode_chunk(in, out, state, density_lion_decode_read_form(in, state)));

    state->chunksCount += DENSITY_LION_CHUNKS_PER_PROCESS_UNIT;
}

DENSITY_FORCE_INLINE bool density_lion_decode_chunk_step_by_step(density_memory_location *restrict readMemoryLocation, density_memory_teleport *restrict in, density_memory_location *restrict out, density_lion_decode_state *restrict state) {
    density_byte* startPointer = readMemoryLocation->pointer;
    DENSITY_LION_FORM form = density_lion_decode_read_form(readMemoryLocation, state);
    readMemoryLocation->available_bytes -= (readMemoryLocation->pointer - startPointer);
    switch (form) {
        case DENSITY_LION_FORM_CHUNK_DICTIONARY_B:  // Potential end marker, we need 2 bytes for a chunk dictionary hash, if remaining bytes < 2 + 2 bytes then this form is the last one
            if (density_unlikely(density_memory_teleport_available_bytes_reserved(in, state->endDataOverhead) < sizeof(uint32_t)))
                return false;
            break;
        default:
            break;
    }
    startPointer = readMemoryLocation->pointer;
    density_lion_decode_chunk(readMemoryLocation, out, state, form);
    readMemoryLocation->available_bytes -= (readMemoryLocation->pointer - startPointer);
    return true;
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_DECODE_STATE density_lion_decode_init(density_lion_decode_state *state, const density_main_header_parameters parameters, const uint_fast8_t endDataOverhead) {
    state->chunksCount = 0;
    state->efficiencyChecked = false;
    state->shift = 0;
    density_lion_dictionary_reset(&state->dictionary);

    state->parameters = parameters;
    density_byte resetDictionaryCycleShift = state->parameters.as_bytes[0];
    if (resetDictionaryCycleShift)
        state->resetCycle = (uint_fast64_t) (1 << resetDictionaryCycleShift) - 1;

    state->endDataOverhead = endDataOverhead;

    density_lion_form_model_init(&state->formData);

    state->lastHash = 0;
    state->lastChunk = 0;

    return exitProcess(state, DENSITY_LION_DECODE_PROCESS_CHECK_BLOCK_STATE, DENSITY_KERNEL_DECODE_STATE_READY);
}

#define DENSITY_LION_DECODE_CONTINUE

#include "kernel_lion_decode_template.h"

#undef DENSITY_LION_DECODE_CONTINUE

#include "kernel_lion_decode_template.h"
