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
 * 18/10/13 23:48
 */

#include "main_encode.h"

DENSITY_FORCE_INLINE DENSITY_ENCODE_STATE exitProcess(density_encode_state *state, DENSITY_ENCODE_PROCESS process, DENSITY_ENCODE_STATE encodeState) {
    state->process = process;
    return encodeState;
}

DENSITY_FORCE_INLINE DENSITY_ENCODE_STATE density_encode_write_header(density_memory_location *restrict out, density_encode_state *restrict state, const DENSITY_COMPRESSION_MODE compressionMode, const DENSITY_BLOCK_TYPE blockType) {
    if (sizeof(density_main_header) > out->available_bytes)
        return DENSITY_ENCODE_STATE_STALL_ON_OUTPUT;

#if DENSITY_ENABLE_PARALLELIZABLE_DECOMPRESSIBLE_OUTPUT == DENSITY_YES
    density_main_header_parameters parameters = {.as_bytes = {DENSITY_DICTIONARY_PREFERRED_RESET_CYCLE_SHIFT, 0, 0, 0, 0, 0, 0, 0}};
#else
    density_main_header_parameters parameters = {.as_bytes = {0, 0, 0, 0, 0, 0, 0, 0}};
#endif

    state->totalWritten += density_main_header_write(out, compressionMode, blockType, parameters);

    return DENSITY_ENCODE_STATE_READY;
}

DENSITY_FORCE_INLINE DENSITY_ENCODE_STATE density_encode_write_footer(density_memory_location *restrict out, density_encode_state *restrict state) {
    if (sizeof(density_main_footer) > out->available_bytes)
        return DENSITY_ENCODE_STATE_STALL_ON_OUTPUT;

    state->totalWritten += density_main_footer_write(out);

    return DENSITY_ENCODE_STATE_READY;
}

DENSITY_FORCE_INLINE void density_encode_update_totals(density_memory_teleport *restrict in, density_memory_location *restrict out, density_encode_state *restrict state, const uint_fast64_t availableInBefore, const uint_fast64_t availableOutBefore) {
    state->totalRead += availableInBefore - density_memory_teleport_available(in);
    state->totalWritten += availableOutBefore - out->available_bytes;
}

DENSITY_FORCE_INLINE DENSITY_ENCODE_STATE density_encode_init(density_memory_location *restrict out, density_encode_state *restrict state, const DENSITY_COMPRESSION_MODE mode, const DENSITY_BLOCK_TYPE blockType, void *(*mem_alloc)(size_t)) {
    DENSITY_ENCODE_STATE encodeState;
    state->compressionMode = mode;
    state->blockType = blockType;

    state->totalRead = 0;
    state->totalWritten = 0;

#if DENSITY_WRITE_MAIN_HEADER == DENSITY_YES
    if ((encodeState = density_encode_write_header(out, state, mode, blockType)))
        return exitProcess(state, DENSITY_ENCODE_PROCESS_WRITE_HEADER, encodeState);
#endif

    switch (mode) {
        case DENSITY_COMPRESSION_MODE_COPY:
            density_block_encode_init(&state->blockEncodeState, DENSITY_COMPRESSION_MODE_COPY, blockType, NULL, NULL, NULL, NULL);
            break;

        case DENSITY_COMPRESSION_MODE_CHAMELEON_ALGORITHM:
            density_block_encode_init(&state->blockEncodeState, DENSITY_COMPRESSION_MODE_CHAMELEON_ALGORITHM, blockType, mem_alloc(sizeof(density_chameleon_encode_state)), (void *) density_chameleon_encode_init, (void *) density_chameleon_encode_continue, (void *) density_chameleon_encode_finish);
            break;

        case DENSITY_COMPRESSION_MODE_MANDALA_ALGORITHM:
            density_block_encode_init(&state->blockEncodeState, DENSITY_COMPRESSION_MODE_MANDALA_ALGORITHM, blockType, mem_alloc(sizeof(density_mandala_encode_state)), (void *) density_mandala_encode_init, (void *) density_mandala_encode_process, (void *) density_mandala_encode_finish);
            break;
    }

    return exitProcess(state, DENSITY_ENCODE_PROCESS_WRITE_BLOCKS, DENSITY_ENCODE_STATE_READY);
}

DENSITY_FORCE_INLINE DENSITY_ENCODE_STATE density_encode_continue(density_memory_teleport *restrict in, density_memory_location *restrict out, density_encode_state *restrict state) {
    DENSITY_BLOCK_ENCODE_STATE blockEncodeState;
    uint_fast64_t availableInBefore;
    uint_fast64_t availableOutBefore;

    // Dispatch
    switch (state->process) {
        case DENSITY_ENCODE_PROCESS_WRITE_BLOCKS:
            goto write_blocks;
        default:
            return DENSITY_ENCODE_STATE_ERROR;
    }

    write_blocks:
    availableInBefore = density_memory_teleport_available(in);
    availableOutBefore = out->available_bytes;

    blockEncodeState = density_block_encode_continue(in, out, &state->blockEncodeState);
    density_encode_update_totals(in, out, state, availableInBefore, availableOutBefore);

    switch (blockEncodeState) {
        case DENSITY_BLOCK_ENCODE_STATE_READY:
            break;
        case DENSITY_BLOCK_ENCODE_STATE_STALL_ON_INPUT:
            return exitProcess(state, DENSITY_ENCODE_PROCESS_WRITE_BLOCKS, DENSITY_ENCODE_STATE_STALL_ON_INPUT);
        case DENSITY_BLOCK_ENCODE_STATE_STALL_ON_OUTPUT:
            return exitProcess(state, DENSITY_ENCODE_PROCESS_WRITE_BLOCKS, DENSITY_ENCODE_STATE_STALL_ON_OUTPUT);
        case DENSITY_BLOCK_ENCODE_STATE_ERROR:
            return DENSITY_ENCODE_STATE_ERROR;
    }
    goto write_blocks;
}

DENSITY_FORCE_INLINE DENSITY_ENCODE_STATE density_encode_finish(density_memory_teleport *restrict in, density_memory_location *restrict out, density_encode_state *restrict state, void (*mem_free)(void *)) {
    DENSITY_ENCODE_STATE encodeState;
    DENSITY_BLOCK_ENCODE_STATE blockEncodeState;
    uint_fast64_t availableInBefore;
    uint_fast64_t availableOutBefore;

    // Dispatch
    switch (state->process) {
        case DENSITY_ENCODE_PROCESS_WRITE_BLOCKS:
            goto write_blocks;
        case DENSITY_ENCODE_PROCESS_WRITE_FOOTER:
            goto write_footer;
        default:
            return DENSITY_ENCODE_STATE_ERROR;
    }

    write_blocks:
    availableInBefore = density_memory_teleport_available(in);
    availableOutBefore = out->available_bytes;

    blockEncodeState = density_block_encode_finish(in, out, &state->blockEncodeState);
    density_encode_update_totals(in, out, state, availableInBefore, availableOutBefore);
    switch (blockEncodeState) {
        case DENSITY_BLOCK_ENCODE_STATE_READY:
            break;
        case DENSITY_BLOCK_ENCODE_STATE_STALL_ON_OUTPUT:
            return exitProcess(state, DENSITY_ENCODE_PROCESS_WRITE_BLOCKS, DENSITY_ENCODE_STATE_STALL_ON_OUTPUT);
        default:
            return DENSITY_ENCODE_STATE_ERROR;
    }

    write_footer:
#if DENSITY_WRITE_MAIN_FOOTER == DENSITY_YES
    if((encodeState = density_encode_write_footer(out, state)))
        return exitProcess(state, DENSITY_ENCODE_PROCESS_WRITE_FOOTER, encodeState);
#endif
    mem_free(state->blockEncodeState.kernelEncodeState);

    return DENSITY_ENCODE_STATE_READY;
}