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
 * 07/12/13 15:49
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

#include "kernel_cheetah_decode.h"

DENSITY_FORCE_INLINE DENSITY_KERNEL_DECODE_STATE exitProcess(density_cheetah_decode_state *state, DENSITY_CHEETAH_DECODE_PROCESS process, DENSITY_KERNEL_DECODE_STATE kernelDecodeState) {
    state->process = process;
    return kernelDecodeState;
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_DECODE_STATE density_cheetah_decode_check_state(density_memory_location *restrict out, density_cheetah_decode_state *restrict state) {
    if (out->available_bytes < (DENSITY_CHEETAH_MAXIMUM_DECOMPRESSED_UNIT_SIZE << DENSITY_CHEETAH_DECODE_ITERATIONS_SHIFT))
        return DENSITY_KERNEL_DECODE_STATE_STALL_ON_OUTPUT;

    switch (state->signaturesCount) {
        case DENSITY_CHEETAH_PREFERRED_EFFICIENCY_CHECK_SIGNATURES:
            if (state->efficiencyChecked ^ 0x1) {
                state->efficiencyChecked = 1;
                return DENSITY_KERNEL_DECODE_STATE_INFO_EFFICIENCY_CHECK;
            }
            break;
        case DENSITY_CHEETAH_PREFERRED_BLOCK_SIGNATURES:
            state->signaturesCount = 0;
            state->efficiencyChecked = 0;

            if (state->resetCycle)
                state->resetCycle--;
            else {
                density_byte resetDictionaryCycleShift = state->parameters.as_bytes[0];
                if (resetDictionaryCycleShift) {
                    density_cheetah_dictionary_reset(&state->dictionary);
                    state->resetCycle = (uint_fast64_t) (1 << resetDictionaryCycleShift) - 1;
                }
            }

            return DENSITY_KERNEL_DECODE_STATE_INFO_NEW_BLOCK;
        default:
            break;
    }
    return DENSITY_KERNEL_DECODE_STATE_READY;
}

DENSITY_FORCE_INLINE void density_cheetah_decode_read_signature(density_memory_location *restrict in, density_cheetah_decode_state *restrict state) {
    state->signature = DENSITY_LITTLE_ENDIAN_64(*(density_cheetah_signature *) (in->pointer));
    in->pointer += sizeof(density_cheetah_signature);
    state->shift = 0;
    state->signaturesCount++;
}

DENSITY_FORCE_INLINE void density_cheetah_decode_read_compressed_chunk(uint32_t *restrict hash, density_memory_location *restrict in) {
    *hash = *(uint16_t *) (in->pointer);
    in->pointer += sizeof(uint16_t);
}

DENSITY_FORCE_INLINE void density_cheetah_decode_read_uncompressed_chunk(uint32_t *restrict chunk, density_memory_location *restrict in) {
    *chunk = *(uint32_t *) (in->pointer);
    in->pointer += sizeof(uint32_t);
}

DENSITY_FORCE_INLINE void density_cheetah_decode_predicted_chunk(uint32_t *restrict hash, density_memory_location *restrict out, density_cheetah_decode_state *restrict state) {
    uint32_t *chunk = (uint32_t *) (&state->dictionary.prediction_entries[state->lastHash]);
    DENSITY_CHEETAH_HASH_ALGORITHM(*hash, DENSITY_LITTLE_ENDIAN_32(*chunk));

    *(uint32_t *) (out->pointer) = *chunk;
    out->pointer += sizeof(uint32_t);
}

DENSITY_FORCE_INLINE void density_cheetah_write_decompressed_chunk(const uint32_t *restrict chunk, density_memory_location *restrict out, density_cheetah_decode_state *restrict state) {
    *(uint32_t *) (out->pointer) = *chunk;
    out->pointer += sizeof(uint32_t);

    (&state->dictionary.prediction_entries[state->lastHash])->next_chunk_prediction = *chunk;
}

DENSITY_FORCE_INLINE void density_cheetah_decode_compressed_chunk_a(const uint32_t *restrict hash, density_memory_location *restrict out, density_cheetah_decode_state *restrict state) {
    uint32_t *chunk = (uint32_t *) (&state->dictionary.entries[DENSITY_LITTLE_ENDIAN_16(*hash)]);

    density_cheetah_write_decompressed_chunk(chunk, out, state);
}

DENSITY_FORCE_INLINE void density_cheetah_decode_compressed_chunk_b(const uint32_t *restrict hash, density_memory_location *restrict out, density_cheetah_decode_state *restrict state) {
    density_cheetah_dictionary_entry *entry = &state->dictionary.entries[DENSITY_LITTLE_ENDIAN_16(*hash)];
    *(uint64_t *) entry = *((uint32_t *) entry + 1) | ((uint64_t) (*(uint32_t *) entry) << 32);

    density_cheetah_write_decompressed_chunk((uint32_t *) entry, out, state);
}

DENSITY_FORCE_INLINE void density_cheetah_decode_uncompressed_chunk(uint32_t *restrict hash, const uint32_t *restrict chunk, density_memory_location *restrict out, density_cheetah_decode_state *restrict state) {
    density_cheetah_dictionary_entry *entry;

    DENSITY_CHEETAH_HASH_ALGORITHM(*hash, DENSITY_LITTLE_ENDIAN_32(*chunk));
    entry = &state->dictionary.entries[*hash];
    *(uint64_t *) entry = *chunk | ((uint64_t) entry->chunk_a << 32);

    density_cheetah_write_decompressed_chunk(chunk, out, state);
}


DENSITY_FORCE_INLINE void density_cheetah_decode_kernel(density_memory_location *restrict in, density_memory_location *restrict out, const uint8_t mode, density_cheetah_decode_state *restrict state) {
    uint32_t hash = 0;
    uint32_t chunk;

    switch (mode) {
        DENSITY_CASE_GENERATOR_4_4_COMBINED(\
            density_cheetah_decode_predicted_chunk(&hash, out, state);, \
            DENSITY_CHEETAH_SIGNATURE_FLAG_PREDICTED, \
            density_cheetah_decode_read_compressed_chunk(&hash, in);\
            density_cheetah_decode_compressed_chunk_a(&hash, out, state);, \
            DENSITY_CHEETAH_SIGNATURE_FLAG_MAP_A, \
            density_cheetah_decode_read_compressed_chunk(&hash, in);\
            density_cheetah_decode_compressed_chunk_b(&hash, out, state);, \
            DENSITY_CHEETAH_SIGNATURE_FLAG_MAP_B, \
            density_cheetah_decode_read_uncompressed_chunk(&chunk, in);\
            density_cheetah_decode_uncompressed_chunk(&hash, &chunk, out, state);, \
            DENSITY_CHEETAH_SIGNATURE_FLAG_CHUNK, \
            state->lastHash = (uint16_t) hash;, \
            2\
        );
        default:
            break;
    }

    state->lastHash = (uint16_t) hash;
}

DENSITY_FORCE_INLINE void density_cheetah_decode_process_data(density_memory_location *restrict in, density_memory_location *restrict out, density_cheetah_decode_state *restrict state) {
    while (density_likely(state->shift != density_bitsizeof(density_cheetah_signature))) {
        density_cheetah_decode_kernel(in, out, (uint8_t const) ((state->signature >> state->shift) & 0xFF), state);
        state->shift += 8;
    }
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_DECODE_STATE density_cheetah_decode_init(density_cheetah_decode_state *state, const density_main_header_parameters parameters, const uint_fast8_t endDataOverhead) {
    state->signaturesCount = 0;
    state->efficiencyChecked = 0;
    density_cheetah_dictionary_reset(&state->dictionary);

    state->parameters = parameters;
    density_byte resetDictionaryCycleShift = state->parameters.as_bytes[0];
    if (resetDictionaryCycleShift)
        state->resetCycle = (uint_fast64_t) (1 << resetDictionaryCycleShift) - 1;

    state->endDataOverhead = endDataOverhead;

    state->lastHash = 0;

    return exitProcess(state, DENSITY_CHEETAH_DECODE_PROCESS_CHECK_SIGNATURE_STATE, DENSITY_KERNEL_DECODE_STATE_READY);
}

#define DENSITY_CHEETAH_DECODE_CONTINUE
#define GENERIC_NAME(name) name ## continue

#include "kernel_cheetah_generic_decode.h"

#undef GENERIC_NAME
#undef DENSITY_CHEETAH_DECODE_CONTINUE

#define GENERIC_NAME(name) name ## finish

#include "kernel_cheetah_generic_decode.h"

#undef GENERIC_NAME