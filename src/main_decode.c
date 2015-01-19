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
 * 19/10/13 00:06
 */

#include "main_decode.h"

DENSITY_FORCE_INLINE DENSITY_DECODE_STATE exitProcess(density_decode_state *state, DENSITY_DECODE_PROCESS process, DENSITY_DECODE_STATE decodeState) {
    state->process = process;
    return decodeState;
}

DENSITY_FORCE_INLINE DENSITY_DECODE_STATE density_decode_read_header(density_memory_teleport *restrict in, density_decode_state *restrict state) {
    density_memory_location *readLocation;
    if (!(readLocation = density_memory_teleport_read_reserved(in, sizeof(density_main_header), DENSITY_DECODE_END_DATA_OVERHEAD)))
        return DENSITY_DECODE_STATE_STALL_ON_INPUT;

    state->totalRead += density_main_header_read(readLocation, &state->header);

    return DENSITY_DECODE_STATE_READY;
}

DENSITY_FORCE_INLINE DENSITY_DECODE_STATE density_decode_read_footer(density_memory_teleport *restrict in, density_decode_state *restrict state) {
    density_memory_location *readLocation;
    if (!(readLocation = density_memory_teleport_read(in, sizeof(density_main_footer))))
        return DENSITY_DECODE_STATE_STALL_ON_INPUT;

    state->totalRead += density_main_footer_read(readLocation, &state->footer);

    return DENSITY_DECODE_STATE_READY;
}

DENSITY_FORCE_INLINE void density_decode_update_totals(density_memory_teleport *restrict in, density_memory_location *restrict out, density_decode_state *restrict state, const uint_fast64_t inAvailableBefore, const uint_fast64_t outAvailableBefore) {
    state->totalRead += inAvailableBefore - density_memory_teleport_available_reserved(in, DENSITY_DECODE_END_DATA_OVERHEAD);
    state->totalWritten += outAvailableBefore - out->available_bytes;
}

DENSITY_FORCE_INLINE DENSITY_DECODE_STATE density_decode_init(density_memory_teleport *in, density_decode_state *restrict state, void *(*mem_alloc)(size_t)) {
    DENSITY_DECODE_STATE decodeState;
    state->totalRead = 0;
    state->totalWritten = 0;

#if DENSITY_WRITE_MAIN_HEADER == DENSITY_YES
    if ((decodeState = density_decode_read_header(in, state)))
        return exitProcess(state, DENSITY_DECODE_PROCESS_READ_HEADER, decodeState);
#endif

    switch (state->header.compressionMode) {
        case DENSITY_COMPRESSION_MODE_COPY:
            density_block_decode_init(&state->blockDecodeState, DENSITY_COMPRESSION_MODE_COPY, (DENSITY_BLOCK_TYPE) state->header.blockType, state->header.parameters, DENSITY_DECODE_END_DATA_OVERHEAD, NULL, NULL, NULL, NULL);
            break;

        case DENSITY_COMPRESSION_MODE_CHAMELEON_ALGORITHM:
            density_block_decode_init(&state->blockDecodeState, DENSITY_COMPRESSION_MODE_CHAMELEON_ALGORITHM, (DENSITY_BLOCK_TYPE) state->header.blockType, state->header.parameters, DENSITY_DECODE_END_DATA_OVERHEAD, mem_alloc(sizeof(density_chameleon_decode_state)), (void *) density_chameleon_decode_init, (void *) density_chameleon_decode_continue, (void *) density_chameleon_decode_finish);
            break;

        case DENSITY_COMPRESSION_MODE_MANDALA_ALGORITHM:
            density_block_decode_init(&state->blockDecodeState, DENSITY_COMPRESSION_MODE_MANDALA_ALGORITHM, (DENSITY_BLOCK_TYPE) state->header.blockType, state->header.parameters, DENSITY_DECODE_END_DATA_OVERHEAD, mem_alloc(sizeof(density_mandala_decode_state)), (void *) density_mandala_decode_init, (void *) density_mandala_decode_process, (void *) density_mandala_decode_finish);
            break;

        default:
            return DENSITY_DECODE_STATE_ERROR;
    }

    return exitProcess(state, DENSITY_DECODE_PROCESS_READ_BLOCKS, DENSITY_DECODE_STATE_READY);
}

DENSITY_FORCE_INLINE DENSITY_DECODE_STATE density_decode_continue(density_memory_teleport *restrict in, density_memory_location *restrict out, density_decode_state *restrict state) {
    DENSITY_BLOCK_DECODE_STATE blockDecodeState;
    uint_fast64_t inAvailableBefore;
    uint_fast64_t outAvailableBefore;

    switch (state->process) {
        case DENSITY_DECODE_PROCESS_READ_BLOCKS:
            goto read_blocks;
        default:
            return DENSITY_DECODE_STATE_ERROR;
    }

    read_blocks:
    inAvailableBefore = density_memory_teleport_available_reserved(in, DENSITY_DECODE_END_DATA_OVERHEAD);
    outAvailableBefore = out->available_bytes;

    blockDecodeState = density_block_decode_continue(in, out, &state->blockDecodeState);
    density_decode_update_totals(in, out, state, inAvailableBefore, outAvailableBefore);

    switch (blockDecodeState) {
        case DENSITY_BLOCK_DECODE_STATE_READY:
            break;
        case DENSITY_BLOCK_DECODE_STATE_STALL_ON_INPUT:
            return exitProcess(state, DENSITY_DECODE_PROCESS_READ_BLOCKS, DENSITY_DECODE_STATE_STALL_ON_INPUT);
        case DENSITY_BLOCK_DECODE_STATE_STALL_ON_OUTPUT:
            return exitProcess(state, DENSITY_DECODE_PROCESS_READ_BLOCKS, DENSITY_DECODE_STATE_STALL_ON_OUTPUT);
        case DENSITY_BLOCK_DECODE_STATE_ERROR:
            return DENSITY_DECODE_STATE_ERROR;
    }

    goto read_blocks;
}

DENSITY_FORCE_INLINE DENSITY_DECODE_STATE density_decode_finish(density_memory_teleport *restrict in, density_memory_location *restrict out, density_decode_state *restrict state, void (*mem_free)(void *)) {
    DENSITY_DECODE_STATE decodeState;
    DENSITY_BLOCK_DECODE_STATE blockDecodeState;
    uint_fast64_t inAvailableBefore;
    uint_fast64_t outAvailableBefore;

    switch (state->process) {
        case DENSITY_DECODE_PROCESS_READ_BLOCKS:
            goto read_blocks;
        case DENSITY_DECODE_PROCESS_READ_FOOTER:
            goto read_footer;
        default:
            return DENSITY_DECODE_STATE_ERROR;
    }

    read_blocks:
    inAvailableBefore = density_memory_teleport_available_reserved(in, DENSITY_DECODE_END_DATA_OVERHEAD);
    outAvailableBefore = out->available_bytes;

    blockDecodeState = density_block_decode_finish(in, out, &state->blockDecodeState);
    density_decode_update_totals(in, out, state, inAvailableBefore, outAvailableBefore);

    switch (blockDecodeState) {
        case DENSITY_BLOCK_DECODE_STATE_READY:
            break;
        case DENSITY_BLOCK_DECODE_STATE_STALL_ON_OUTPUT:
            return exitProcess(state, DENSITY_DECODE_PROCESS_READ_BLOCKS, DENSITY_DECODE_STATE_STALL_ON_OUTPUT);
        default:
            return DENSITY_DECODE_STATE_ERROR;
    }

    read_footer:
#if DENSITY_WRITE_MAIN_FOOTER == DENSITY_YES
    if ((decodeState = density_decode_read_footer(in, state)))
        return decodeState;
#endif
    mem_free(state->blockDecodeState.kernelDecodeState);

    return DENSITY_DECODE_STATE_READY;
}