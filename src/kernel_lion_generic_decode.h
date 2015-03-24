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

DENSITY_FORCE_INLINE DENSITY_KERNEL_DECODE_STATE GENERIC_NAME(density_lion_decode_)(density_memory_teleport *restrict in, density_memory_location *restrict out, density_lion_decode_state *restrict state) {
    DENSITY_KERNEL_DECODE_STATE returnState;
    density_memory_location *readMemoryLocation;

    // Dispatch
    switch (state->process) {
        case DENSITY_LION_DECODE_PROCESS_CHECK_BLOCK_STATE:
            goto check_block_state;
        case DENSITY_LION_DECODE_PROCESS_CHECK_OUTPUT_SIZE:
            goto check_output_size;
        case DENSITY_LION_DECODE_PROCESS_UNIT:
            goto process_unit;
        default:
            return DENSITY_KERNEL_DECODE_STATE_ERROR;
    }

    // Check block state
    check_block_state:
    if (density_unlikely(!state->shift)) {
        if (density_unlikely(returnState = density_lion_decode_check_block_state(state)))
            return exitProcess(state, DENSITY_LION_DECODE_PROCESS_CHECK_BLOCK_STATE, returnState);
    }

    // Check output size
    check_output_size:
    if (density_unlikely(out->available_bytes < DENSITY_LION_DECODE_MINIMUM_OUTPUT_LOOKAHEAD))
        return exitProcess(state, DENSITY_LION_DECODE_PROCESS_CHECK_OUTPUT_SIZE, DENSITY_KERNEL_DECODE_STATE_STALL_ON_OUTPUT);

    // Try to read the next processing unit
    process_unit:
    if (density_unlikely(!(readMemoryLocation = density_memory_teleport_read_reserved(in, DENSITY_LION_DECODE_MAX_BYTES_TO_READ_FOR_PROCESS_UNIT, state->endDataOverhead))))
#ifdef DENSITY_LION_DECODE_CONTINUE
        return exitProcess(state, DENSITY_LION_DECODE_PROCESS_UNIT, DENSITY_KERNEL_DECODE_STATE_STALL_ON_INPUT);
#else
        goto step_by_step;
#endif

    density_byte *readMemoryLocationPointerBefore = readMemoryLocation->pointer;
    density_lion_decode_process_unit(readMemoryLocation, out, state);
    readMemoryLocation->available_bytes -= (readMemoryLocation->pointer - readMemoryLocationPointerBefore);
    out->available_bytes -= DENSITY_LION_PROCESS_UNIT_SIZE;

    // New loop
    goto check_block_state;

    // Try to read and process units step by step
    step_by_step:
    readMemoryLocation = density_memory_teleport_read_remaining_reserved(in, state->endDataOverhead);
    uint_fast8_t iterations = DENSITY_LION_CHUNKS_PER_PROCESS_UNIT;
    while (iterations--) {
        bool proceed = density_lion_decode_chunk_step_by_step(readMemoryLocation, in, out, state);
        if(density_unlikely(!proceed))
            goto finish;
        out->available_bytes -= sizeof(uint32_t);
    }

    // New loop
    goto check_block_state;

    finish:
    density_memory_teleport_copy(in, out, density_memory_teleport_available_bytes_reserved(in, state->endDataOverhead));

    return DENSITY_KERNEL_DECODE_STATE_READY;
}