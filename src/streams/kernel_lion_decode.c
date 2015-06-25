/*
 * Centaurean Density
 *
 * Copyright (c) 2015, Guillaume Voirin
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

DENSITY_FORCE_INLINE DENSITY_KERNEL_DECODE_STATE density_lion_decode_exit_process(density_lion_decode_state *state, DENSITY_LION_DECODE_PROCESS process, DENSITY_KERNEL_DECODE_STATE kernelDecodeState) {
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

DENSITY_FORCE_INLINE DENSITY_LION_DECODE_STEP_BY_STEP_STATUS density_lion_decode_chunk_step_by_step(density_memory_location *restrict readMemoryLocation, density_memory_teleport *restrict in, density_memory_location *restrict out, density_lion_decode_state *restrict state) {
    density_byte *startPointer = readMemoryLocation->pointer;
    if (density_unlikely(!state->shift))
        density_lion_decode_bulk_read_signature((const uint8_t **) &readMemoryLocation->pointer, &state->signature);
    DENSITY_LION_FORM form = density_lion_decode_bulk_read_form((const uint8_t **) &readMemoryLocation->pointer, &state->signature, &state->shift, &state->formData);
    readMemoryLocation->available_bytes -= (readMemoryLocation->pointer - startPointer);
    switch (form) {
        case DENSITY_LION_FORM_PLAIN:  // Potential end marker
            if (density_unlikely(density_memory_teleport_available_bytes_reserved(in, state->endDataOverhead) <= sizeof(uint32_t)))
                return DENSITY_LION_DECODE_STEP_BY_STEP_STATUS_END_MARKER;
            break;
        default:
            break;
    }
    if (out->available_bytes < sizeof(uint32_t))
        return DENSITY_LION_DECODE_STEP_BY_STEP_STATUS_STALL_ON_OUTPUT;
    startPointer = readMemoryLocation->pointer;
    density_lion_decode_bulk_4((const uint8_t **) &readMemoryLocation->pointer, &out->pointer, &state->lastHash, &state->dictionary, &state->formData, form);
    state->chunksCount++;
    readMemoryLocation->available_bytes -= (readMemoryLocation->pointer - startPointer);
    return DENSITY_LION_DECODE_STEP_BY_STEP_STATUS_PROCEED;
}

DENSITY_WINDOWS_EXPORT DENSITY_FORCE_INLINE DENSITY_KERNEL_DECODE_STATE density_lion_decode_init(density_lion_decode_state *state, const density_main_header_parameters parameters, const uint_fast8_t endDataOverhead) {
    state->chunksCount = 0;
    state->efficiencyChecked = false;
    state->shift = 0;
    density_lion_dictionary_reset(&state->dictionary);

    state->parameters = parameters;
    density_byte resetDictionaryCycleShift = state->parameters.as_bytes[0];
    if (resetDictionaryCycleShift)
        state->resetCycle = (uint_fast64_t) (1 << resetDictionaryCycleShift) - 1;

    state->endDataOverhead = endDataOverhead;

    density_lion_form_data *const form_data = &state->formData;
    density_lion_form_model_init(form_data);
    void (*attachments[DENSITY_LION_NUMBER_OF_FORMS])(const uint8_t **, uint8_t **, uint_fast16_t *, void *const, uint16_t *const, uint32_t *const) = {(void (*)(const uint8_t **, uint8_t **, uint_fast16_t *, void *const, uint16_t *const, uint32_t *const)) &density_lion_decode_bulk_prediction_a, (void (*)(const uint8_t **, uint8_t **, uint_fast16_t *, void *const, uint16_t *const, uint32_t *const)) &density_lion_decode_bulk_prediction_b, (void (*)(const uint8_t **, uint8_t **, uint_fast16_t *, void *const, uint16_t *const, uint32_t *const)) &density_lion_decode_bulk_prediction_c, (void (*)(const uint8_t **, uint8_t **, uint_fast16_t *, void *const, uint16_t *const, uint32_t *const)) &density_lion_decode_bulk_dictionary_a, (void (*)(const uint8_t **, uint8_t **, uint_fast16_t *, void *const, uint16_t *const, uint32_t *const)) &density_lion_decode_bulk_dictionary_b, (void (*)(const uint8_t **, uint8_t **, uint_fast16_t *, void *const, uint16_t *const, uint32_t *const)) &density_lion_decode_bulk_dictionary_c, (void (*)(const uint8_t **, uint8_t **, uint_fast16_t *, void *const, uint16_t *const, uint32_t *const)) &density_lion_decode_bulk_dictionary_d, (void (*)(const uint8_t **, uint8_t **, uint_fast16_t *, void *const, uint16_t *const, uint32_t *const)) &density_lion_decode_bulk_plain};
    density_lion_form_model_attach(form_data, attachments);

    state->lastHash = 0;
    state->lastChunk = 0;

    return density_lion_decode_exit_process(state, DENSITY_LION_DECODE_PROCESS_CHECK_BLOCK_STATE, DENSITY_KERNEL_DECODE_STATE_READY);
}

#include "kernel_lion_decode_template.h"

#define DENSITY_LION_DECODE_FINISH

#include "kernel_lion_decode_template.h"