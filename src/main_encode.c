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

DENSITY_FORCE_INLINE DENSITY_ENCODE_STATE density_encode_write_header(density_memory_location *restrict out, density_encode_state *restrict state, const DENSITY_COMPRESSION_MODE compressionMode, const DENSITY_BLOCK_TYPE blockType) {
    if (sizeof(density_main_header) > out->available_bytes)
        return DENSITY_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER;

#if DENSITY_ENABLE_PARALLELIZABLE_DECOMPRESSIBLE_OUTPUT == DENSITY_YES
    density_main_header_parameters parameters = {.as_bytes = {DENSITY_DICTIONARY_PREFERRED_RESET_CYCLE_SHIFT, 0, 0, 0, 0, 0, 0, 0}};
#else
    density_main_header_parameters parameters = {.as_bytes = {0, 0, 0, 0, 0, 0, 0, 0}};
#endif

    state->totalWritten += density_main_header_write(out, compressionMode, blockType, parameters);

    state->process = DENSITY_ENCODE_PROCESS_WRITE_BLOCKS;

    return DENSITY_ENCODE_STATE_READY;
}

DENSITY_FORCE_INLINE DENSITY_ENCODE_STATE density_encode_write_footer(density_memory_location *restrict out, density_encode_state *restrict state) {
    if (sizeof(density_main_footer) > out->available_bytes)
        return DENSITY_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER;

    state->totalWritten += density_main_footer_write(out);

    state->process = DENSITY_ENCODE_PROCESS_FINISHED;

    return DENSITY_ENCODE_STATE_READY;
}

DENSITY_FORCE_INLINE void density_encode_update_totals(density_teleport *restrict in, density_memory_location *restrict out, density_encode_state *restrict state, const uint_fast64_t availableInBefore, const uint_fast64_t availableOutBefore) {
    state->totalRead += availableInBefore - in->directMemoryLocation->available_bytes;
    state->totalWritten += availableOutBefore - out->available_bytes;
}

DENSITY_FORCE_INLINE DENSITY_ENCODE_STATE density_encode_init(density_memory_location *restrict out, density_encode_state *restrict state, const DENSITY_COMPRESSION_MODE mode, const DENSITY_ENCODE_OUTPUT_TYPE encodeOutputType, const DENSITY_BLOCK_TYPE blockType) {
    state->compressionMode = mode;
    state->blockType = blockType;
    state->encodeOutputType = encodeOutputType;

    state->totalRead = 0;
    state->totalWritten = 0;

    switch (mode) {
        case DENSITY_COMPRESSION_MODE_COPY:
            density_block_encode_init(&state->blockEncodeState, DENSITY_BLOCK_MODE_COPY, blockType, NULL, NULL, NULL, NULL);
            break;

        case DENSITY_COMPRESSION_MODE_CHAMELEON_ALGORITHM:
            density_block_encode_init(&state->blockEncodeState, DENSITY_BLOCK_MODE_KERNEL, blockType, malloc(sizeof(density_chameleon_encode_state)), (void *) density_chameleon_encode_init, (void *) density_chameleon_encode_process, (void *) density_chameleon_encode_finish);
            break;

        case DENSITY_COMPRESSION_MODE_MANDALA_ALGORITHM:
            //density_block_encode_init(&state->blockEncodeState, DENSITY_BLOCK_MODE_KERNEL, blockType, malloc(sizeof(density_mandala_encode_state)), (void *) density_mandala_encode_init, (void *) density_mandala_encode_process, (void *) density_mandala_encode_finish);
            break;
    }

    switch (state->encodeOutputType) {
        case DENSITY_ENCODE_OUTPUT_TYPE_WITHOUT_HEADER:
        case DENSITY_ENCODE_OUTPUT_TYPE_WITHOUT_HEADER_NOR_FOOTER:
            state->process = DENSITY_ENCODE_PROCESS_WRITE_BLOCKS;
            return DENSITY_ENCODE_STATE_READY;
        default:
            return density_encode_write_header(out, state, mode, blockType);
    }
}

DENSITY_FORCE_INLINE DENSITY_ENCODE_STATE density_encode_process(density_teleport *restrict in, density_memory_location *restrict out, density_encode_state *restrict state, const density_bool flush) {
    DENSITY_BLOCK_ENCODE_STATE blockEncodeState;
    uint_fast64_t availableInBefore;
    uint_fast64_t availableOutBefore;

    while (true) {
        availableInBefore = in->directMemoryLocation->available_bytes;
        availableOutBefore = out->available_bytes;

        switch (state->process) {
            case DENSITY_ENCODE_PROCESS_WRITE_BLOCKS:
                blockEncodeState = density_block_encode_process(in, out, &state->blockEncodeState, flush);
                density_encode_update_totals(in, out, state, availableInBefore, availableOutBefore);

                switch (blockEncodeState) {
                    case DENSITY_BLOCK_ENCODE_STATE_READY:
                        state->process = DENSITY_ENCODE_PROCESS_WRITE_FOOTER;
                        return DENSITY_ENCODE_STATE_READY;

                    case DENSITY_BLOCK_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER:
                        return DENSITY_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER;

                    case DENSITY_BLOCK_ENCODE_STATE_STALL_ON_INPUT_BUFFER:
                        return DENSITY_ENCODE_STATE_STALL_ON_INPUT_BUFFER;

                    case DENSITY_BLOCK_ENCODE_STATE_ERROR:
                        return DENSITY_ENCODE_STATE_ERROR;
                }
                break;

            default:
                return
                        DENSITY_ENCODE_STATE_ERROR;
        }
    }
}

DENSITY_FORCE_INLINE DENSITY_ENCODE_STATE density_encode_finish(density_memory_location *restrict out, density_encode_state *restrict state) {
    if (state->process ^ DENSITY_ENCODE_PROCESS_WRITE_FOOTER)
        return DENSITY_ENCODE_STATE_ERROR;

    switch (state->compressionMode) {
        default:
            density_block_encode_finish(&state->blockEncodeState);
            free(state->blockEncodeState.kernelEncodeState);
            break;

        case DENSITY_COMPRESSION_MODE_COPY:
            break;
    }

    switch (state->encodeOutputType) {
        case DENSITY_ENCODE_OUTPUT_TYPE_WITHOUT_FOOTER:
        case DENSITY_ENCODE_OUTPUT_TYPE_WITHOUT_HEADER_NOR_FOOTER:
            return DENSITY_ENCODE_STATE_READY;
        default:
            return density_encode_write_footer(out, state);
    }
}