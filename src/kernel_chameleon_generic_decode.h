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
 * 8/03/15 03:16
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

DENSITY_FORCE_INLINE DENSITY_KERNEL_DECODE_STATE GENERIC_NAME(density_chameleon_decode_) (density_memory_teleport *restrict in, density_memory_location *restrict out, density_chameleon_decode_state *restrict state) {
    DENSITY_KERNEL_DECODE_STATE returnState;
    density_memory_location *readMemoryLocation;

    // Dispatch
    switch (state->process) {
        case DENSITY_CHAMELEON_DECODE_PROCESS_CHECK_SIGNATURE_STATE:
            goto check_signature_state;
        case DENSITY_CHAMELEON_DECODE_PROCESS_READ_PROCESSING_UNIT:
            goto read_processing_unit;
        default:
            return DENSITY_KERNEL_DECODE_STATE_ERROR;
    }

    check_signature_state:
    if ((returnState = density_chameleon_decode_check_state(out, state)))
        return exitProcess(state, DENSITY_CHAMELEON_DECODE_PROCESS_CHECK_SIGNATURE_STATE, returnState);

    // Try to read the next processing unit
    read_processing_unit:
    if (!(readMemoryLocation = density_memory_teleport_read_reserved(in, DENSITY_CHAMELEON_MAXIMUM_COMPRESSED_UNIT_SIZE << DENSITY_CHAMELEON_DECODE_ITERATIONS_SHIFT, state->endDataOverhead)))
#ifdef DENSITY_CHAMELEON_DECODE_CONTINUE
        return exitProcess(state, DENSITY_CHAMELEON_DECODE_PROCESS_READ_PROCESSING_UNIT, DENSITY_KERNEL_DECODE_STATE_STALL_ON_INPUT);
#else
        goto step_by_step;
#endif

    uint_fast8_t iterations = (1 << DENSITY_CHAMELEON_DECODE_ITERATIONS_SHIFT);
    density_byte *readMemoryLocationPointerBefore = readMemoryLocation->pointer;
    while(iterations --) {
        // Decode the signature (endian processing)
        density_chameleon_decode_read_signature(readMemoryLocation, state);

        // Process body
        density_chameleon_decode_process_data(readMemoryLocation, out, state);
    }
    readMemoryLocation->available_bytes -= (readMemoryLocation->pointer - readMemoryLocationPointerBefore);
    out->available_bytes -= ((density_bitsizeof(density_chameleon_signature) * sizeof(uint32_t)) << DENSITY_CHAMELEON_DECODE_ITERATIONS_SHIFT);

    // New loop
    goto check_signature_state;

    // Try to read and process signature and body, step by step
    step_by_step:
    if (!(readMemoryLocation = density_memory_teleport_read_reserved(in, sizeof(density_chameleon_signature), state->endDataOverhead)))
        goto finish;
    density_chameleon_decode_read_signature(readMemoryLocation, state);
    readMemoryLocation->available_bytes -= sizeof(density_chameleon_signature);

    while (state->shift != density_bitsizeof(density_chameleon_signature)) {
        if (density_chameleon_decode_test_compressed(state)) {
            uint16_t chunk;
            if (!(readMemoryLocation = density_memory_teleport_read_reserved(in, sizeof(uint16_t), state->endDataOverhead)))
                return DENSITY_KERNEL_DECODE_STATE_ERROR;
            density_chameleon_decode_read_compressed_chunk(&chunk, readMemoryLocation);
            readMemoryLocation->available_bytes -= sizeof(uint16_t);
            density_chameleon_decode_compressed_chunk(&chunk, out, state);
        } else {
            uint32_t chunk;
            if (!(readMemoryLocation = density_memory_teleport_read_reserved(in, sizeof(uint32_t), state->endDataOverhead)))
                goto finish;
            density_chameleon_decode_read_uncompressed_chunk(&chunk, readMemoryLocation);
            readMemoryLocation->available_bytes -= sizeof(uint32_t);
            density_chameleon_decode_uncompressed_chunk(&chunk, out, state);
        }
        out->available_bytes -= sizeof(uint32_t);
        state->shift++;
    }

    // New loop
    goto check_signature_state;

    finish:
    density_memory_teleport_copy(in, out, density_memory_teleport_available_bytes_reserved(in, state->endDataOverhead));

    return DENSITY_KERNEL_DECODE_STATE_READY;
}
