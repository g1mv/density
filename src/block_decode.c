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

DENSITY_FORCE_INLINE DENSITY_BLOCK_DECODE_STATE density_block_decode_read_block_header(density_memory_teleport *restrict in, density_block_decode_state *restrict state) {
    density_memory_location* readLocation;
    if(!(readLocation = density_memory_teleport_read(in, sizeof(density_block_header))))
        return DENSITY_BLOCK_DECODE_STATE_STALL_ON_INPUT_BUFFER;

    state->currentBlockData.inStart = state->totalRead;
    state->currentBlockData.outStart = state->totalWritten;

    state->totalRead += density_block_header_read(readLocation, &state->lastBlockHeader);

    state->currentMode = state->targetMode;
    state->process = DENSITY_BLOCK_DECODE_PROCESS_READ_DATA;

    return DENSITY_BLOCK_DECODE_STATE_READY;
}

DENSITY_FORCE_INLINE DENSITY_BLOCK_DECODE_STATE density_block_decode_read_block_footer(density_memory_teleport *restrict in, density_block_decode_state *restrict state) {
    density_memory_location* readLocation;
    if(!(readLocation = density_memory_teleport_read(in, sizeof(density_block_footer))))
        return DENSITY_BLOCK_DECODE_STATE_STALL_ON_INPUT_BUFFER;

    state->totalRead += density_block_footer_read(readLocation, &state->lastBlockFooter);

    return DENSITY_BLOCK_DECODE_STATE_READY;
}

DENSITY_FORCE_INLINE DENSITY_BLOCK_DECODE_STATE density_block_decode_read_block_mode_marker(density_memory_teleport *restrict in, density_block_decode_state *restrict state) {
    density_memory_location* readLocation;
    if(!(readLocation = density_memory_teleport_read(in, sizeof(density_mode_marker))))
        return DENSITY_BLOCK_DECODE_STATE_STALL_ON_INPUT_BUFFER;

    state->totalRead += density_block_mode_marker_read(readLocation, &state->lastModeMarker);

    state->currentMode = (DENSITY_COMPRESSION_MODE) state->lastModeMarker.activeBlockMode;
    state->process = DENSITY_BLOCK_DECODE_PROCESS_READ_DATA;

    return DENSITY_BLOCK_DECODE_STATE_READY;
}

DENSITY_FORCE_INLINE void density_block_decode_update_totals(density_memory_teleport *restrict in, density_memory_location *restrict out, density_block_decode_state *restrict state, const uint_fast64_t inAvailableBefore, const uint_fast64_t outAvailableBefore) {
    state->totalRead += inAvailableBefore - density_memory_teleport_available(in);
    state->totalWritten += outAvailableBefore - out->available_bytes;
}

DENSITY_FORCE_INLINE DENSITY_BLOCK_DECODE_STATE density_block_decode_init(density_block_decode_state *restrict state, const DENSITY_COMPRESSION_MODE mode, const DENSITY_BLOCK_TYPE blockType, const density_main_header_parameters parameters, const uint_fast32_t endDataOverhead, void *kernelState, DENSITY_KERNEL_DECODE_STATE (*kernelInit)(void *, const density_main_header_parameters, const uint_fast64_t), DENSITY_KERNEL_DECODE_STATE (*kernelProcess)(density_memory_teleport *, density_memory_location *, void *, const density_bool), DENSITY_KERNEL_DECODE_STATE (*kernelFinish)(void *)) {
    state->process = DENSITY_BLOCK_DECODE_PROCESS_READ_BLOCK_HEADER;
    state->targetMode = mode;
    state->currentMode = mode;
    state->blockMode = mode ? DENSITY_BLOCK_MODE_KERNEL : DENSITY_BLOCK_MODE_COPY;
    state->blockType = blockType;

    state->totalRead = 0;
    state->totalWritten = 0;
    state->endDataOverhead = (state->blockType == DENSITY_BLOCK_TYPE_DEFAULT ? sizeof(density_block_footer) : 0) + endDataOverhead;

    switch (state->blockMode) {
        case DENSITY_BLOCK_MODE_KERNEL:
            state->kernelDecodeState = kernelState;
            state->kernelDecodeInit = kernelInit;
            state->kernelDecodeProcess = kernelProcess;
            state->kernelDecodeFinish = kernelFinish;

            state->kernelDecodeInit(state->kernelDecodeState, parameters, state->endDataOverhead);
            break;
        default:
            break;
    }

    return DENSITY_BLOCK_DECODE_STATE_READY;
}

DENSITY_FORCE_INLINE DENSITY_BLOCK_DECODE_STATE density_block_decode_process(density_memory_teleport *restrict in, density_memory_location *restrict out, density_block_decode_state *restrict state, const density_bool flush) {
    DENSITY_BLOCK_DECODE_STATE decodeState;
    DENSITY_KERNEL_DECODE_STATE hashDecodeState;
    uint_fast64_t inAvailableBefore;
    uint_fast64_t outAvailableBefore;
    uint_fast64_t blockRemaining;
    uint_fast64_t inRemaining;
    uint_fast64_t positionIncrement;
    uint_fast64_t outRemaining;

    while (true) {
        switch (state->process) {
            case DENSITY_BLOCK_DECODE_PROCESS_READ_BLOCK_HEADER:
                if ((decodeState = density_block_decode_read_block_header(in, state)))
                    return decodeState;
                break;

            case DENSITY_BLOCK_DECODE_PROCESS_READ_BLOCK_MODE_MARKER:
                if ((decodeState = density_block_decode_read_block_mode_marker(in, state)))
                    return decodeState;
                break;

            case DENSITY_BLOCK_DECODE_PROCESS_READ_BLOCK_FOOTER:
                if (state->blockType == DENSITY_BLOCK_TYPE_DEFAULT) if ((decodeState = density_block_decode_read_block_footer(in, state)))
                    return decodeState;
                state->process = DENSITY_BLOCK_DECODE_PROCESS_READ_BLOCK_HEADER;
                break;

            case DENSITY_BLOCK_DECODE_PROCESS_READ_LAST_BLOCK_FOOTER:
                if (state->blockType == DENSITY_BLOCK_TYPE_DEFAULT) if ((decodeState = density_block_decode_read_block_footer(in, state)))
                    return decodeState;
                state->process = DENSITY_BLOCK_DECODE_PROCESS_FINISHED;
                return DENSITY_BLOCK_DECODE_STATE_READY;

            case DENSITY_BLOCK_DECODE_PROCESS_READ_DATA:
                inAvailableBefore = density_memory_teleport_available(in);
                outAvailableBefore = out->available_bytes;
                switch (state->blockMode) {
                    case DENSITY_BLOCK_MODE_COPY:
                        blockRemaining = (uint_fast64_t) DENSITY_PREFERRED_COPY_BLOCK_SIZE - (state->totalWritten - state->currentBlockData.outStart);
                        inRemaining = density_memory_teleport_available(in);
                        outRemaining = out->available_bytes;

                        if (inRemaining <= outRemaining) {
                            if (blockRemaining <= inRemaining)
                                goto copy_until_end_of_block;
                            else {
                                if (flush && inRemaining <= state->endDataOverhead) {
                                    state->process = DENSITY_BLOCK_DECODE_PROCESS_READ_LAST_BLOCK_FOOTER;
                                } else if (!inRemaining) {
                                    return DENSITY_BLOCK_DECODE_STATE_STALL_ON_INPUT_BUFFER;
                                } else {
                                    positionIncrement = inRemaining - (flush ? state->endDataOverhead : 0);
                                    density_memory_teleport_copy(in, out, positionIncrement);
                                    density_block_decode_update_totals(in, out, state, inAvailableBefore, outAvailableBefore);
                                }
                            }
                        } else {
                            if (blockRemaining <= outRemaining)
                                goto copy_until_end_of_block;
                            else {
                                if (outRemaining) {
                                    density_memory_teleport_copy(in, out, outRemaining);
                                    density_block_decode_update_totals(in, out, state, inAvailableBefore, outAvailableBefore);
                                } else
                                    return DENSITY_BLOCK_DECODE_STATE_STALL_ON_OUTPUT_BUFFER;
                            }
                        }
                        goto exit;

                    copy_until_end_of_block:
                        density_memory_teleport_copy(in, out, blockRemaining);
                        density_block_decode_update_totals(in, out, state, inAvailableBefore, outAvailableBefore);
                        state->process = DENSITY_BLOCK_DECODE_PROCESS_READ_BLOCK_FOOTER;

                    exit:
                        break;

                    case DENSITY_BLOCK_MODE_KERNEL:
                        hashDecodeState = state->kernelDecodeProcess(in, out, state->kernelDecodeState, flush);
                        density_block_decode_update_totals(in, out, state, inAvailableBefore, outAvailableBefore);

                        switch (hashDecodeState) {
                            case DENSITY_KERNEL_DECODE_STATE_STALL_ON_INPUT_BUFFER:
                                return DENSITY_BLOCK_DECODE_STATE_STALL_ON_INPUT_BUFFER;

                            case DENSITY_KERNEL_DECODE_STATE_STALL_ON_OUTPUT_BUFFER:
                                return DENSITY_BLOCK_DECODE_STATE_STALL_ON_OUTPUT_BUFFER;

                            case DENSITY_KERNEL_DECODE_STATE_INFO_NEW_BLOCK:
                                state->process = DENSITY_BLOCK_DECODE_PROCESS_READ_BLOCK_FOOTER;
                                break;

                            case DENSITY_KERNEL_DECODE_STATE_INFO_EFFICIENCY_CHECK:
                                state->process = DENSITY_BLOCK_DECODE_PROCESS_READ_BLOCK_MODE_MARKER;
                                break;

                            case DENSITY_KERNEL_DECODE_STATE_FINISHED:
                                state->process = DENSITY_BLOCK_DECODE_PROCESS_READ_LAST_BLOCK_FOOTER;
                                break;

                            case DENSITY_KERNEL_DECODE_STATE_READY:
                                break;

                            default:
                                return DENSITY_BLOCK_DECODE_STATE_ERROR;
                        }
                        break;

                    default:
                        return DENSITY_BLOCK_DECODE_STATE_ERROR;
                }
                break;

            default:
                return DENSITY_BLOCK_DECODE_STATE_ERROR;
        }
    }
}

DENSITY_FORCE_INLINE DENSITY_BLOCK_DECODE_STATE density_block_decode_finish(density_block_decode_state *restrict state) {
    if (state->process ^ DENSITY_BLOCK_DECODE_PROCESS_FINISHED)
        return DENSITY_BLOCK_DECODE_STATE_ERROR;

    state->kernelDecodeFinish(state->kernelDecodeState);

    return DENSITY_BLOCK_DECODE_STATE_READY;
}