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

#include "kernel_cheetah_encode.h"

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE exitProcess(density_cheetah_encode_state *state, DENSITY_CHEETAH_ENCODE_PROCESS process, DENSITY_KERNEL_ENCODE_STATE kernelEncodeState) {
    state->process = process;
    return kernelEncodeState;
}

DENSITY_FORCE_INLINE void density_cheetah_encode_write_to_signature(density_cheetah_encode_state *state, uint_fast8_t flag) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    (state->proximitySignature) |= ((uint64_t) flag) << state->shift;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    (state->proximitySignature) |= ((uint64_t) flag) << ((56 - (state->shift & ~0x7)) + (state->shift & 0x7));
#endif
}

DENSITY_FORCE_INLINE void density_cheetah_encode_prepare_new_signature(density_memory_location *restrict out, density_cheetah_encode_state *restrict state) {
    state->signaturesCount++;
    state->shift = 0;
    state->signature = (density_cheetah_signature *) (out->pointer);
    state->proximitySignature = 0;

    out->pointer += sizeof(density_cheetah_signature);
    out->available_bytes -= sizeof(density_cheetah_signature);
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE density_cheetah_encode_prepare_new_block(density_memory_location *restrict out, density_cheetah_encode_state *restrict state) {
    if (DENSITY_CHEETAH_MAXIMUM_COMPRESSED_UNIT_SIZE > out->available_bytes)
        return DENSITY_KERNEL_ENCODE_STATE_STALL_ON_OUTPUT;

    switch (state->signaturesCount) {
        case DENSITY_CHEETAH_PREFERRED_EFFICIENCY_CHECK_SIGNATURES:
            if (state->efficiencyChecked ^ 0x1) {
                state->efficiencyChecked = 1;
                return DENSITY_KERNEL_ENCODE_STATE_INFO_EFFICIENCY_CHECK;
            }
            break;
        case DENSITY_CHEETAH_PREFERRED_BLOCK_SIGNATURES:
            state->signaturesCount = 0;
            state->efficiencyChecked = 0;

#if DENSITY_ENABLE_PARALLELIZABLE_DECOMPRESSIBLE_OUTPUT == DENSITY_YES
            if (state->resetCycle)
                state->resetCycle--;
            else {
                density_cheetah_dictionary_reset(&state->dictionary);
                state->resetCycle = DENSITY_DICTIONARY_PREFERRED_RESET_CYCLE - 1;
            }
#endif

            return DENSITY_KERNEL_ENCODE_STATE_INFO_NEW_BLOCK;
        default:
            break;
    }
    density_cheetah_encode_prepare_new_signature(out, state);

    return DENSITY_KERNEL_ENCODE_STATE_READY;
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE density_cheetah_encode_check_state(density_memory_location *restrict out, density_cheetah_encode_state *restrict state) {
    DENSITY_KERNEL_ENCODE_STATE returnState;

    switch (state->shift) {
        case density_bitsizeof(density_cheetah_signature):
            *state->signature = state->proximitySignature;
            if ((returnState = density_cheetah_encode_prepare_new_block(out, state)))
                return returnState;
            break;
        default:
            break;
    }

    return DENSITY_KERNEL_ENCODE_STATE_READY;
}

DENSITY_FORCE_INLINE void density_cheetah_encode_kernel(density_memory_location *restrict out, uint32_t *restrict hash, const uint32_t chunk, density_cheetah_encode_state *restrict state) {
    DENSITY_CHEETAH_HASH_ALGORITHM(*hash, DENSITY_LITTLE_ENDIAN_32(chunk));
    uint32_t *predictedChunk = &(state->dictionary.prediction_entries[state->lastHash].next_chunk_prediction);

    if (*predictedChunk ^ chunk) {
        density_cheetah_dictionary_entry *found = &state->dictionary.entries[*hash];
        uint32_t *found_a = &found->chunk_a;
        if (*found_a ^ chunk) {
            uint32_t *found_b = &found->chunk_b;
            if (*found_b ^ chunk) {
                density_cheetah_encode_write_to_signature(state, DENSITY_CHEETAH_SIGNATURE_FLAG_CHUNK);
                *(uint32_t *) (out->pointer) = chunk;
                out->pointer += sizeof(uint32_t);
            } else {
                density_cheetah_encode_write_to_signature(state, DENSITY_CHEETAH_SIGNATURE_FLAG_MAP_B);
                *(uint16_t *) (out->pointer) = DENSITY_LITTLE_ENDIAN_16(*hash);
                out->pointer += sizeof(uint16_t);
            }
            *found_b = *found_a;
            *found_a = chunk;
        } else {
            density_cheetah_encode_write_to_signature(state, DENSITY_CHEETAH_SIGNATURE_FLAG_MAP_A);
            *(uint16_t *) (out->pointer) = DENSITY_LITTLE_ENDIAN_16(*hash);
            out->pointer += sizeof(uint16_t);
        }
        *predictedChunk = chunk;
    } else {
        density_cheetah_encode_write_to_signature(state, DENSITY_CHEETAH_SIGNATURE_FLAG_PREDICTED);
    }
    state->lastHash = (uint16_t) *hash;

    state->shift += 2;
}

DENSITY_FORCE_INLINE void density_cheetah_encode_process_chunk(uint64_t *restrict chunk, density_memory_location *restrict in, density_memory_location *restrict out, uint32_t *restrict hash, density_cheetah_encode_state *restrict state) {
    *chunk = *(uint64_t *) (in->pointer);
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    density_cheetah_encode_kernel(out, hash, (uint32_t) (*chunk & 0xFFFFFFFF), state);
#endif
    density_cheetah_encode_kernel(out, hash, (uint32_t) (*chunk >> 32), state);
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    density_cheetah_encode_kernel(out, hash, (uint32_t) (*chunk & 0xFFFFFFFF), state);
#endif
    in->pointer += sizeof(uint64_t);
}

DENSITY_FORCE_INLINE void density_cheetah_encode_process_unit(uint64_t *restrict chunk, density_memory_location *restrict in, density_memory_location *restrict out, uint32_t *restrict hash, density_cheetah_encode_state *restrict state) {
    DENSITY_UNROLL_8(density_cheetah_encode_process_chunk(chunk, in, out, hash, state));
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE density_cheetah_encode_init(density_cheetah_encode_state *state) {
    state->signaturesCount = 0;
    state->efficiencyChecked = 0;
    density_cheetah_dictionary_reset(&state->dictionary);

#if DENSITY_ENABLE_PARALLELIZABLE_DECOMPRESSIBLE_OUTPUT == DENSITY_YES
    state->resetCycle = DENSITY_DICTIONARY_PREFERRED_RESET_CYCLE - 1;
#endif

    state->lastHash = 0;

    return exitProcess(state, DENSITY_CHEETAH_ENCODE_PROCESS_PREPARE_NEW_BLOCK, DENSITY_KERNEL_ENCODE_STATE_READY);
}

#define DENSITY_CHEETAH_ENCODE_CONTINUE
#define GENERIC_NAME(name) name ## continue

#include "kernel_cheetah_generic_encode.h"

#undef GENERIC_NAME
#undef DENSITY_CHEETAH_ENCODE_CONTINUE

#define GENERIC_NAME(name) name ## finish

#include "kernel_cheetah_generic_encode.h"

#undef GENERIC_NAME