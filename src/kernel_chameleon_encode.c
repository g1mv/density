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
 * 24/10/13 12:01
 *
 * -------------------
 * Chameleon algorithm
 * -------------------
 *
 * Author(s)
 * Guillaume Voirin (https://github.com/gpnuma)
 *
 * Description
 * Hash based superfast kernel
 */

#include "kernel_chameleon_encode.h"

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE density_chameleon_encode_exit_process(density_chameleon_encode_state *state, DENSITY_CHAMELEON_ENCODE_PROCESS process, DENSITY_KERNEL_ENCODE_STATE kernelEncodeState) {
    state->process = process;
    return kernelEncodeState;
}

DENSITY_FORCE_INLINE void density_chameleon_encode_write_to_signature(density_chameleon_encode_state *state) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    (state->proximitySignature) |= ((uint64_t) DENSITY_CHAMELEON_SIGNATURE_FLAG_MAP) << state->shift;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    (state->proximitySignature) |= ((uint64_t) DENSITY_CHAMELEON_SIGNATURE_FLAG_MAP) << ((56 - (state->shift & ~0x7)) + (state->shift & 0x7));
#endif
}

DENSITY_FORCE_INLINE void density_chameleon_encode_prepare_new_signature(density_memory_location *restrict out, density_chameleon_encode_state *restrict state) {
    state->signaturesCount++;
    state->shift = 0;
    state->signature = (density_chameleon_signature *) (out->pointer);
    state->proximitySignature = 0;

    out->pointer += sizeof(density_chameleon_signature);
    out->available_bytes -= sizeof(density_chameleon_signature);
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE density_chameleon_encode_prepare_new_block(density_memory_location *restrict out, density_chameleon_encode_state *restrict state) {
    if (DENSITY_CHAMELEON_MAXIMUM_COMPRESSED_UNIT_SIZE > out->available_bytes)
        return DENSITY_KERNEL_ENCODE_STATE_STALL_ON_OUTPUT;

    switch (state->signaturesCount) {
        case DENSITY_CHAMELEON_PREFERRED_EFFICIENCY_CHECK_SIGNATURES:
            if (state->efficiencyChecked ^ 0x1) {
                state->efficiencyChecked = 1;
                return DENSITY_KERNEL_ENCODE_STATE_INFO_EFFICIENCY_CHECK;
            }
            break;
        case DENSITY_CHAMELEON_PREFERRED_BLOCK_SIGNATURES:
            state->signaturesCount = 0;
            state->efficiencyChecked = 0;

#if DENSITY_ENABLE_PARALLELIZABLE_DECOMPRESSIBLE_OUTPUT == DENSITY_YES
            if (state->resetCycle)
                state->resetCycle--;
            else {
                density_chameleon_dictionary_reset();

                state-> resetCycle = DENSITY_DICTIONARY_PREFERRED_RESET_CYCLE - 1;
            }
#endif

            return DENSITY_KERNEL_ENCODE_STATE_INFO_NEW_BLOCK;
        default:
            break;
    }
    density_chameleon_encode_prepare_new_signature(out, state);

    return DENSITY_KERNEL_ENCODE_STATE_READY;
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE density_chameleon_encode_check_state(density_memory_location *restrict out, density_chameleon_encode_state *restrict state) {
    DENSITY_KERNEL_ENCODE_STATE returnState;

    switch (state->shift) {
        case density_bitsizeof(density_chameleon_signature):
            *(state->signature) = state->proximitySignature;
            if ((returnState = density_chameleon_encode_prepare_new_block(out, state)))
                return returnState;
            break;
        default:
            break;
    }

    return DENSITY_KERNEL_ENCODE_STATE_READY;
}

DENSITY_FORCE_INLINE void density_chameleon_encode_kernel(density_memory_location *restrict out, const uint16_t hash, const uint32_t chunk, density_chameleon_encode_state *restrict state) {
    density_chameleon_dictionary_entry *found = &state->dictionary.entries[hash];

    if (chunk ^ *(uint32_t *) found) {
        found->as_uint32_t = chunk;
        *(uint32_t *) (out->pointer) = chunk;
        out->pointer += sizeof(uint32_t);
    } else {
        density_chameleon_encode_write_to_signature(state);
        *(uint16_t *) (out->pointer) = DENSITY_LITTLE_ENDIAN_16(hash);
        out->pointer += sizeof(uint16_t);
    }

    state->shift++;
}

DENSITY_FORCE_INLINE void density_chameleon_encode_process_chunk(density_memory_location *restrict in, density_memory_location *restrict out, density_chameleon_encode_state *restrict state) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    const uint128_t chunk = *(uint128_t * )(in->pointer);
    const uint128_t hash_group = (uint32_t) (((uint32_t) chunk * DENSITY_CHAMELEON_HASH_MULTIPLIER)) | (uint64_t) (((uint64_t) (chunk & DENSITY_MASK_32_64) * DENSITY_CHAMELEON_HASH_MULTIPLIER)) | (((chunk & DENSITY_MASK_64_96) * DENSITY_CHAMELEON_HASH_MULTIPLIER) & DENSITY_MASK_64_96) | (((chunk & DENSITY_MASK_96_128) * DENSITY_CHAMELEON_HASH_MULTIPLIER));

    density_chameleon_encode_kernel(out, *((uint16_t *) &hash_group + 1), (uint32_t) chunk, state);
    density_chameleon_encode_kernel(out, *((uint16_t *) &hash_group + 3), *((uint32_t *) &chunk + 1), state);
    density_chameleon_encode_kernel(out, *((uint16_t *) &hash_group + 5), *((uint32_t *) &chunk + 2), state);
    density_chameleon_encode_kernel(out, *((uint16_t *) &hash_group + 7), *((uint32_t *) &chunk + 3), state);
#else
    const uint64_t chunk_a = *(uint64_t *) (in->pointer);
    const uint32_t element_a = (uint32_t) (chunk_a >> density_bitsizeof(uint32_t));
    const uint32_t element_b = (uint32_t) chunk_a;
    density_chameleon_encode_kernel(out, DENSITY_CHAMELEON_HASH_ALGORITHM(DENSITY_LITTLE_ENDIAN_32(element_a)), element_a, state);
    density_chameleon_encode_kernel(out, DENSITY_CHAMELEON_HASH_ALGORITHM(DENSITY_LITTLE_ENDIAN_32(element_b)), element_b, state);

    const uint64_t chunk_b = *((uint64_t *) in->pointer + 1);
    const uint32_t element_c = (uint32_t) (chunk_b >> density_bitsizeof(uint32_t));
    const uint32_t element_d = (uint32_t) chunk_b;
    density_chameleon_encode_kernel(out, DENSITY_CHAMELEON_HASH_ALGORITHM(DENSITY_LITTLE_ENDIAN_32(element_c)), element_c, state);
    density_chameleon_encode_kernel(out, DENSITY_CHAMELEON_HASH_ALGORITHM(DENSITY_LITTLE_ENDIAN_32(element_d)), element_d, state);
#endif

    in->pointer += sizeof(uint128_t);
}

DENSITY_FORCE_INLINE void density_chameleon_encode_process_unit(density_memory_location *restrict in, density_memory_location *restrict out, density_chameleon_encode_state *restrict state) {
    DENSITY_UNROLL_8(density_chameleon_encode_process_chunk(in, out, state));
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE density_chameleon_encode_init(density_chameleon_encode_state *state) {
    state->signaturesCount = 0;
    state->efficiencyChecked = 0;
    density_chameleon_dictionary_reset(&state->dictionary);

#if DENSITY_ENABLE_PARALLELIZABLE_DECOMPRESSIBLE_OUTPUT == DENSITY_YES
    state->resetCycle = DENSITY_DICTIONARY_PREFERRED_RESET_CYCLE - 1;
#endif

    return density_chameleon_encode_exit_process(state, DENSITY_CHAMELEON_ENCODE_PROCESS_PREPARE_NEW_BLOCK, DENSITY_KERNEL_ENCODE_STATE_READY);
}

#include "kernel_chameleon_encode_template.h"

#define DENSITY_CHAMELEON_ENCODE_FINISH

#include "kernel_chameleon_encode_template.h"