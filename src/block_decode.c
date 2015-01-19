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
 * 19/10/13 00:20
 */

#include "block_decode.h"

DENSITY_FORCE_INLINE DENSITY_BLOCK_DECODE_STATE exitProcess(density_block_decode_state *state, DENSITY_BLOCK_DECODE_PROCESS process, DENSITY_BLOCK_DECODE_STATE blockDecodeState) {
    state->process = process;
    return blockDecodeState;
}

DENSITY_FORCE_INLINE DENSITY_BLOCK_DECODE_STATE density_block_decode_read_block_header(density_memory_teleport *restrict in, density_block_decode_state *restrict state) {
    density_memory_location *readLocation;
    if (!(readLocation = density_memory_teleport_read_reserved(in, sizeof(density_block_header), state->endDataOverhead)))
        return DENSITY_BLOCK_DECODE_STATE_STALL_ON_INPUT;

    state->currentBlockData.inStart = state->totalRead;
    state->currentBlockData.outStart = state->totalWritten;

    state->totalRead += density_block_header_read(readLocation, &state->lastBlockHeader);

    state->currentMode = state->targetMode;

    return DENSITY_BLOCK_DECODE_STATE_READY;
}

DENSITY_FORCE_INLINE DENSITY_BLOCK_DECODE_STATE density_block_decode_read_block_footer(density_memory_teleport *restrict in, density_block_decode_state *restrict state) {
    density_memory_location *readLocation;
    if (!(readLocation = density_memory_teleport_read(in, sizeof(density_block_footer))))
        return DENSITY_BLOCK_DECODE_STATE_STALL_ON_INPUT;

    state->totalRead += density_block_footer_read(readLocation, &state->lastBlockFooter);

    return DENSITY_BLOCK_DECODE_STATE_READY;
}

DENSITY_FORCE_INLINE DENSITY_BLOCK_DECODE_STATE density_block_decode_read_block_mode_marker(density_memory_teleport *restrict in, density_block_decode_state *restrict state) {
    density_memory_location *readLocation;
    if (!(readLocation = density_memory_teleport_read_reserved(in, sizeof(density_mode_marker), state->endDataOverhead)))
        return DENSITY_BLOCK_DECODE_STATE_STALL_ON_INPUT;

    state->totalRead += density_block_mode_marker_read(readLocation, &state->lastModeMarker);

    state->currentMode = (DENSITY_COMPRESSION_MODE) state->lastModeMarker.activeBlockMode;

    return DENSITY_BLOCK_DECODE_STATE_READY;
}

DENSITY_FORCE_INLINE void density_block_decode_update_totals(density_memory_teleport *restrict in, density_memory_location *restrict out, density_block_decode_state *restrict state, const uint_fast64_t inAvailableBefore, const uint_fast64_t outAvailableBefore) {
    state->totalRead += inAvailableBefore - density_memory_teleport_available_reserved(in, state->endDataOverhead);
    state->totalWritten += outAvailableBefore - out->available_bytes;
}

DENSITY_FORCE_INLINE DENSITY_BLOCK_DECODE_STATE density_block_decode_init(density_block_decode_state *restrict state, const DENSITY_COMPRESSION_MODE mode, const DENSITY_BLOCK_TYPE blockType, const density_main_header_parameters parameters, const uint_fast32_t endDataOverhead, void *kernelState, DENSITY_KERNEL_DECODE_STATE (*kernelInit)(void *, const density_main_header_parameters, const uint_fast64_t), DENSITY_KERNEL_DECODE_STATE (*kernelProcess)(density_memory_teleport *, density_memory_location *, void *), DENSITY_KERNEL_DECODE_STATE (*kernelFinish)(density_memory_teleport *, density_memory_location *, void *)) {
    state->targetMode = mode;
    state->currentMode = mode;
    state->blockType = blockType;

    state->totalRead = 0;
    state->totalWritten = 0;
    state->endDataOverhead = (state->blockType == DENSITY_BLOCK_TYPE_WITH_HASHSUM_INTEGRITY_CHECK ? sizeof(density_block_footer) : 0) + endDataOverhead;

    switch (state->currentMode) {
        case DENSITY_COMPRESSION_MODE_COPY:
            break;

        default:
            state->kernelDecodeState = kernelState;
            state->kernelDecodeInit = kernelInit;
            state->kernelDecodeProcess = kernelProcess;
            state->kernelDecodeFinish = kernelFinish;

            state->kernelDecodeInit(state->kernelDecodeState, parameters, state->endDataOverhead);
            break;
    }

    return exitProcess(state, DENSITY_BLOCK_DECODE_PROCESS_READ_BLOCK_HEADER, DENSITY_BLOCK_DECODE_STATE_READY);
}

DENSITY_FORCE_INLINE DENSITY_BLOCK_DECODE_STATE density_block_decode_continue(density_memory_teleport *restrict in, density_memory_location *restrict out, density_block_decode_state *restrict state) {
    DENSITY_KERNEL_DECODE_STATE kernelDecodeState;
    DENSITY_BLOCK_DECODE_STATE blockDecodeState;
    uint_fast64_t inAvailableBefore;
    uint_fast64_t outAvailableBefore;
    uint_fast64_t blockRemaining;
    uint_fast64_t inRemaining;
    uint_fast64_t outRemaining;

    // Dispatch
    switch (state->process) {
        case DENSITY_BLOCK_DECODE_PROCESS_READ_BLOCK_HEADER:
            goto read_block_header;
        case DENSITY_BLOCK_DECODE_PROCESS_READ_BLOCK_MODE_MARKER:
            goto read_mode_marker;
        case DENSITY_BLOCK_DECODE_PROCESS_READ_DATA:
            goto read_data;
        case DENSITY_BLOCK_DECODE_PROCESS_READ_BLOCK_FOOTER:
            goto read_block_footer;
        default:
            return DENSITY_BLOCK_DECODE_STATE_ERROR;
    }

    read_mode_marker:
    if ((blockDecodeState = density_block_decode_read_block_mode_marker(in, state)))
        return exitProcess(state, DENSITY_BLOCK_DECODE_PROCESS_READ_BLOCK_MODE_MARKER, blockDecodeState);
    goto read_data;

    read_block_header:
    if ((blockDecodeState = density_block_decode_read_block_header(in, state)))
        return exitProcess(state, DENSITY_BLOCK_DECODE_PROCESS_READ_BLOCK_HEADER, blockDecodeState);

    read_data:
    inAvailableBefore = density_memory_teleport_available_reserved(in, state->endDataOverhead);
    outAvailableBefore = out->available_bytes;
    switch (state->currentMode) {
        case DENSITY_COMPRESSION_MODE_COPY:
            blockRemaining = (uint_fast64_t) DENSITY_PREFERRED_COPY_BLOCK_SIZE - (state->totalWritten - state->currentBlockData.outStart);
            inRemaining = density_memory_teleport_available_reserved(in, state->endDataOverhead);
            outRemaining = out->available_bytes;

            if (inRemaining <= outRemaining) {
                if (blockRemaining <= inRemaining)
                    goto copy_until_end_of_block;
                else {
                    density_memory_teleport_copy(in, out, inRemaining);
                    density_block_decode_update_totals(in, out, state, inAvailableBefore, outAvailableBefore);
                    return exitProcess(state, DENSITY_BLOCK_DECODE_PROCESS_READ_DATA, DENSITY_BLOCK_DECODE_STATE_STALL_ON_INPUT);
                }
            } else {
                if (blockRemaining <= outRemaining)
                    goto copy_until_end_of_block;
                else {
                    density_memory_teleport_copy(in, out, outRemaining);
                    density_block_decode_update_totals(in, out, state, inAvailableBefore, outAvailableBefore);
                    return exitProcess(state, DENSITY_BLOCK_DECODE_PROCESS_READ_DATA, DENSITY_BLOCK_DECODE_STATE_STALL_ON_OUTPUT);
                }
            }

        copy_until_end_of_block:
            density_memory_teleport_copy(in, out, blockRemaining);
            density_block_decode_update_totals(in, out, state, inAvailableBefore, outAvailableBefore);
            goto read_block_footer;

        default:
            kernelDecodeState = state->kernelDecodeProcess(in, out, state->kernelDecodeState);
            density_block_decode_update_totals(in, out, state, inAvailableBefore, outAvailableBefore);

            switch (kernelDecodeState) {
                case DENSITY_KERNEL_DECODE_STATE_STALL_ON_INPUT:
                    return exitProcess(state, DENSITY_BLOCK_DECODE_PROCESS_READ_DATA, DENSITY_BLOCK_DECODE_STATE_STALL_ON_INPUT);
                case DENSITY_KERNEL_DECODE_STATE_STALL_ON_OUTPUT:
                    return exitProcess(state, DENSITY_BLOCK_DECODE_PROCESS_READ_DATA, DENSITY_BLOCK_DECODE_STATE_STALL_ON_OUTPUT);
                case DENSITY_KERNEL_DECODE_STATE_INFO_NEW_BLOCK:
                    goto read_block_header;
                case DENSITY_KERNEL_DECODE_STATE_INFO_EFFICIENCY_CHECK:
                    goto read_mode_marker;
                default:
                    return DENSITY_BLOCK_DECODE_STATE_ERROR;
            }
    }

    read_block_footer:
    if (state->blockType == DENSITY_BLOCK_TYPE_WITH_HASHSUM_INTEGRITY_CHECK) if ((blockDecodeState = density_block_decode_read_block_footer(in, state)))
        return exitProcess(state, DENSITY_BLOCK_DECODE_PROCESS_READ_BLOCK_FOOTER, blockDecodeState);
    goto read_block_header;
}

DENSITY_FORCE_INLINE DENSITY_BLOCK_DECODE_STATE density_block_decode_finish(density_memory_teleport *restrict in, density_memory_location *restrict out, density_block_decode_state *restrict state) {
    DENSITY_KERNEL_DECODE_STATE kernelDecodeState;
    DENSITY_BLOCK_DECODE_STATE blockDecodeState;
    uint_fast64_t inAvailableBefore;
    uint_fast64_t outAvailableBefore;
    uint_fast64_t blockRemaining;
    uint_fast64_t inRemaining;
    uint_fast64_t outRemaining;

    // Dispatch
    switch (state->process) {
        case DENSITY_BLOCK_DECODE_PROCESS_READ_BLOCK_HEADER:
            goto read_block_header;
        case DENSITY_BLOCK_DECODE_PROCESS_READ_BLOCK_MODE_MARKER:
            goto read_mode_marker;
        case DENSITY_BLOCK_DECODE_PROCESS_READ_DATA:
            goto read_data;
        case DENSITY_BLOCK_DECODE_PROCESS_READ_BLOCK_FOOTER:
            goto read_block_footer;
        default:
            return DENSITY_BLOCK_DECODE_STATE_ERROR;
    }

    read_mode_marker:
    if ((blockDecodeState = density_block_decode_read_block_mode_marker(in, state)))
        return exitProcess(state, DENSITY_BLOCK_DECODE_PROCESS_READ_BLOCK_MODE_MARKER, blockDecodeState);
    goto read_data;

    read_block_header:
    if ((blockDecodeState = density_block_decode_read_block_header(in, state)))
        return exitProcess(state, DENSITY_BLOCK_DECODE_PROCESS_READ_BLOCK_HEADER, blockDecodeState);

    read_data:
    inAvailableBefore = density_memory_teleport_available_reserved(in, state->endDataOverhead);
    outAvailableBefore = out->available_bytes;

    switch (state->currentMode) {
        case DENSITY_COMPRESSION_MODE_COPY:
            blockRemaining = (uint_fast64_t) DENSITY_PREFERRED_COPY_BLOCK_SIZE - (state->totalWritten - state->currentBlockData.outStart);
            inRemaining = density_memory_teleport_available_reserved(in, state->endDataOverhead);
            outRemaining = out->available_bytes;

            if (inRemaining <= outRemaining) {
                if (blockRemaining <= inRemaining)
                    goto copy_until_end_of_block;
                else {
                    density_memory_teleport_copy(in, out, inRemaining);
                    density_block_decode_update_totals(in, out, state, inAvailableBefore, outAvailableBefore);
                    goto read_block_footer;
                }
            } else {
                if (blockRemaining <= outRemaining)
                    goto copy_until_end_of_block;
                else {
                    density_memory_teleport_copy(in, out, outRemaining);
                    density_block_decode_update_totals(in, out, state, inAvailableBefore, outAvailableBefore);
                    return exitProcess(state, DENSITY_BLOCK_DECODE_PROCESS_READ_DATA, DENSITY_BLOCK_DECODE_STATE_STALL_ON_OUTPUT);
                }
            }

        copy_until_end_of_block:
            density_memory_teleport_copy(in, out, blockRemaining);
            density_block_decode_update_totals(in, out, state, inAvailableBefore, outAvailableBefore);
            goto read_block_footer;

        default:
            kernelDecodeState = state->kernelDecodeFinish(in, out, state->kernelDecodeState);
            density_block_decode_update_totals(in, out, state, inAvailableBefore, outAvailableBefore);

            switch (kernelDecodeState) {
                case DENSITY_KERNEL_DECODE_STATE_STALL_ON_INPUT:
                    return DENSITY_BLOCK_DECODE_STATE_ERROR;
                case DENSITY_KERNEL_DECODE_STATE_STALL_ON_OUTPUT:
                    return exitProcess(state, DENSITY_BLOCK_DECODE_PROCESS_READ_DATA, DENSITY_BLOCK_DECODE_STATE_STALL_ON_OUTPUT);
                case DENSITY_KERNEL_DECODE_STATE_READY:
                case DENSITY_KERNEL_DECODE_STATE_INFO_NEW_BLOCK:
                    goto read_block_footer;
                case DENSITY_KERNEL_DECODE_STATE_INFO_EFFICIENCY_CHECK:
                    goto read_mode_marker;
                default:
                    return DENSITY_BLOCK_DECODE_STATE_ERROR;
            }
    }

    read_block_footer:
    if (state->blockType == DENSITY_BLOCK_TYPE_WITH_HASHSUM_INTEGRITY_CHECK) if ((blockDecodeState = density_block_decode_read_block_footer(in, state)))
        return blockDecodeState;
    if (density_memory_teleport_available(in))
        goto read_block_header;

    return DENSITY_BLOCK_DECODE_STATE_READY;
}