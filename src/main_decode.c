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

DENSITY_FORCE_INLINE DENSITY_DECODE_STATE density_decode_read_header(density_memory_teleport *restrict in, density_decode_state *restrict state) {
    density_memory_location *readLocation;
    if (!(readLocation = density_memory_teleport_read(in, sizeof(density_main_header))))
        return DENSITY_DECODE_STATE_STALL_ON_INPUT_BUFFER;

    state->totalRead += density_main_header_read(readLocation, &state->header);

    state->process = DENSITY_DECODE_PROCESS_READ_BLOCKS;

    return DENSITY_DECODE_STATE_READY;
}

DENSITY_FORCE_INLINE DENSITY_DECODE_STATE density_decode_read_footer(density_memory_teleport *restrict in, density_decode_state *restrict state) {
    density_memory_location *readLocation;
    if (!(readLocation = density_memory_teleport_read(in, sizeof(density_main_footer))))
        return DENSITY_DECODE_STATE_STALL_ON_INPUT_BUFFER;

    state->totalRead += density_main_footer_read(readLocation, &state->footer);

    state->process = DENSITY_DECODE_PROCESS_FINISHED;

    return DENSITY_DECODE_STATE_READY;
}

DENSITY_FORCE_INLINE void density_decode_update_totals(density_memory_teleport *restrict in, density_memory_location *restrict out, density_decode_state *restrict state, const uint_fast64_t inAvailableBefore, const uint_fast64_t outAvailableBefore) {
    state->totalRead += inAvailableBefore - density_memory_teleport_available(in);
    state->totalWritten += outAvailableBefore - out->available_bytes;
}

DENSITY_FORCE_INLINE DENSITY_DECODE_STATE density_decode_init(density_memory_teleport *in, density_decode_state *restrict state, void *(*mem_alloc)(size_t)) {
    DENSITY_DECODE_STATE decodeState;

    state->totalRead = 0;
    state->totalWritten = 0;

    if ((decodeState = density_decode_read_header(in, state)))
        return decodeState;

    switch (state->header.compressionMode) {
        case DENSITY_COMPRESSION_MODE_COPY:
            density_block_decode_init(&state->blockDecodeState, DENSITY_COMPRESSION_MODE_COPY, (DENSITY_BLOCK_TYPE) state->header.blockType, state->header.parameters, sizeof(density_main_footer), NULL, NULL, NULL, NULL);
            break;

        case DENSITY_COMPRESSION_MODE_CHAMELEON_ALGORITHM:
            density_block_decode_init(&state->blockDecodeState, DENSITY_COMPRESSION_MODE_CHAMELEON_ALGORITHM, (DENSITY_BLOCK_TYPE) state->header.blockType, state->header.parameters, sizeof(density_main_footer), mem_alloc(sizeof(density_chameleon_decode_state)), (void *) density_chameleon_decode_init, (void *) density_chameleon_decode_process, (void *) density_chameleon_decode_finish);
            break;

        case DENSITY_COMPRESSION_MODE_MANDALA_ALGORITHM:
//density_block_decode_init(&state->blockDecodeState, DENSITY_COMPRESSION_MODE_MANDALA_ALGORITHM, (DENSITY_BLOCK_TYPE) state->header.blockType, state->header.parameters, sizeof(density_main_footer), mem_alloc(sizeof(density_mandala_decode_state)), (void *) density_mandala_decode_init, (void *) density_mandala_decode_process, (void *) density_mandala_decode_finish);
            break;

        default:
            return DENSITY_DECODE_STATE_ERROR;
    }

    return DENSITY_DECODE_STATE_READY;
}

DENSITY_FORCE_INLINE DENSITY_DECODE_STATE density_decode_process(density_memory_teleport *restrict in, density_memory_location *restrict out, density_decode_state *restrict state, const density_bool flush) {
    DENSITY_BLOCK_DECODE_STATE blockDecodeState;
    uint_fast64_t inAvailableBefore;
    uint_fast64_t outAvailableBefore;

    while (true) {
        inAvailableBefore = density_memory_teleport_available(in);
        outAvailableBefore = out->available_bytes;

        switch (state->process) {
            case DENSITY_DECODE_PROCESS_READ_BLOCKS:
                blockDecodeState = density_block_decode_process(in, out, &state->blockDecodeState, flush);
                density_decode_update_totals(in, out, state, inAvailableBefore, outAvailableBefore);

                switch (blockDecodeState) {
                    case DENSITY_BLOCK_DECODE_STATE_READY:
                        state->process = DENSITY_DECODE_PROCESS_READ_FOOTER;
                        return DENSITY_DECODE_STATE_READY;

                    case DENSITY_BLOCK_DECODE_STATE_STALL_ON_OUTPUT_BUFFER:
                        return DENSITY_DECODE_STATE_STALL_ON_OUTPUT_BUFFER;

                    case DENSITY_BLOCK_DECODE_STATE_STALL_ON_INPUT_BUFFER:
                        return DENSITY_DECODE_STATE_STALL_ON_INPUT_BUFFER;

                    case DENSITY_BLOCK_DECODE_STATE_ERROR:
                        return DENSITY_DECODE_STATE_ERROR;
                }
                break;

            default:
                return DENSITY_DECODE_STATE_ERROR;
        }
    }
}

DENSITY_FORCE_INLINE DENSITY_DECODE_STATE density_decode_finish(density_memory_teleport *in, density_decode_state *restrict state, void (*mem_free)(void *)) {
    if (state->process ^ DENSITY_DECODE_PROCESS_READ_FOOTER)
        return DENSITY_DECODE_STATE_ERROR;

    switch (state->header.compressionMode) {
        default:
            density_block_decode_finish(&state->blockDecodeState);
            mem_free(state->blockDecodeState.kernelDecodeState);
            break;

        case DENSITY_COMPRESSION_MODE_COPY:
            break;
    }

    return density_decode_read_footer(in, state);
}