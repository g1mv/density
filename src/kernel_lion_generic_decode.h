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
 * 9/03/15 16:59
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

#include <assert.h>
#include "memory_location.h"
#include "kernel_lion_decode.h"
#include "memory_teleport.h"

DENSITY_FORCE_INLINE DENSITY_KERNEL_DECODE_STATE GENERIC_NAME(density_lion_decode_)(density_memory_teleport *restrict in, density_memory_location *restrict out, density_lion_decode_state *restrict state) {
    DENSITY_KERNEL_DECODE_STATE returnState;
    density_memory_location *readMemoryLocation;

    // Dispatch
    switch (state->process) {
        case DENSITY_LION_DECODE_PROCESS_CHECK_SIGNATURE_STATE:
            goto check_signature_state;
        case DENSITY_LION_DECODE_PROCESS_READ_PROCESSING_UNIT:
            goto read_processing_unit;
        default:
            return DENSITY_KERNEL_DECODE_STATE_ERROR;
    }

    check_signature_state:
    if ((DENSITY_LION_MAXIMUM_DECOMPRESSED_UNIT_SIZE << DENSITY_LION_DECODE_ITERATIONS_SHIFT) > out->available_bytes)
        return DENSITY_KERNEL_DECODE_STATE_STALL_ON_OUTPUT;
    if (density_unlikely((returnState = density_lion_decode_check_state(state))))
        return exitProcess(state, DENSITY_LION_DECODE_PROCESS_CHECK_SIGNATURE_STATE, returnState);

    // Try to read the next processing unit
    read_processing_unit:
    if (density_unlikely(!(readMemoryLocation = density_memory_teleport_read_reserved(in, sizeof(density_lion_signature) + (DENSITY_LION_MAXIMUM_COMPRESSED_UNIT_SIZE << DENSITY_LION_DECODE_ITERATIONS_SHIFT), state->endDataOverhead))))
#ifdef DENSITY_LION_DECODE_CONTINUE
        return exitProcess(state, DENSITY_LION_DECODE_PROCESS_READ_PROCESSING_UNIT, DENSITY_KERNEL_DECODE_STATE_STALL_ON_INPUT);
#else
        goto step_by_step;
#endif

    density_byte *readMemoryLocationPointerBefore = readMemoryLocation->pointer;
    density_byte *outPointerBefore = out->pointer;
    uint_fast8_t iterations = (1 << DENSITY_LION_DECODE_ITERATIONS_SHIFT);
    while (iterations--)
        density_lion_decode_kernel(readMemoryLocation, out, state);
    readMemoryLocation->available_bytes -= (readMemoryLocation->pointer - readMemoryLocationPointerBefore);
    out->available_bytes -= (out->pointer - outPointerBefore);

    if(out->available_bytes < 316708 && out->available_bytes > 316508)
        printf("ouch");

    // New loop
    goto check_signature_state;

    // Try to read and process signature and body, step by step
    step_by_step:
    readMemoryLocation = density_memory_teleport_read_remaining_reserved(in, state->endDataOverhead);
    readMemoryLocationPointerBefore = readMemoryLocation->pointer;
    outPointerBefore = out->pointer;
    while (state->shift < 0x10) {  // Maximum signature part size per chunk is 3 + 2 * (1 + 1 + 4) = 15
        if(density_unlikely(!density_lion_decode_unit_step_by_step(readMemoryLocation, in, out, state)))
            goto finish;
    }
    do {
        if(density_unlikely(!density_lion_decode_unit_step_by_step(readMemoryLocation, in, out, state)))
            goto finish;
    } while (density_likely(state->shift > 0x10));
    readMemoryLocation->available_bytes -= (readMemoryLocation->pointer - readMemoryLocationPointerBefore);
    out->available_bytes -= (out->pointer - outPointerBefore);

    // New loop
    goto check_signature_state;

    finish:
    readMemoryLocation->available_bytes -= (readMemoryLocation->pointer - readMemoryLocationPointerBefore);
    out->available_bytes -= (out->pointer - outPointerBefore);
    density_memory_teleport_copy(in, out, density_memory_teleport_available_bytes_reserved(in, state->endDataOverhead));

    return DENSITY_KERNEL_DECODE_STATE_READY;
}