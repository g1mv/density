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

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE density_lion_encode_check_block_state(density_lion_encode_state *restrict state) {
    if(density_unlikely(!(state->chunksCount & (DENSITY_LION_CHUNKS_PER_PROCESS_UNIT_BIG - 1)))) {
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
    }

    return DENSITY_KERNEL_ENCODE_STATE_READY;
}

DENSITY_FORCE_INLINE void density_lion_encode_process_step_unit(density_memory_location *restrict in, density_memory_location *restrict out, density_lion_encode_state *restrict state) {
    uint32_t unit;
    DENSITY_MEMCPY(&unit, in->pointer, sizeof(uint32_t));
    density_lion_encode_bulk_kernel_4(&out->pointer, &state->lastHash, &state->signature, &state->proximitySignature, &state->shift, &state->dictionary, DENSITY_LION_HASH_ALGORITHM(DENSITY_LITTLE_ENDIAN_32(unit)), &state->formData, unit);
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