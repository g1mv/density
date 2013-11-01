/*
 * Centaurean libssc
 * http://www.libssc.net
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

SSC_FORCE_INLINE SSC_BLOCK_DECODE_STATE ssc_block_decode_read_block_header(ssc_byte_buffer *restrict in, ssc_block_decode_state *restrict state) {
    if (in->position + sizeof(ssc_block_header) > in->size)
        return SSC_BLOCK_DECODE_STATE_STALL_ON_INPUT_BUFFER;

    state->currentBlockData.inStart = state->totalRead;
    state->currentBlockData.outStart = state->totalWritten;

    state->totalRead += ssc_block_header_read(in, &state->lastBlockHeader);

    state->process = SSC_BLOCK_DECODE_PROCESS_READ_DATA;

    return SSC_BLOCK_DECODE_STATE_READY;
}

SSC_FORCE_INLINE SSC_BLOCK_DECODE_STATE ssc_block_decode_read_block_footer(ssc_byte_buffer *restrict in, ssc_block_decode_state *restrict state) {
    if (in->position + sizeof(ssc_block_footer) > in->size)
        return SSC_BLOCK_DECODE_STATE_STALL_ON_INPUT_BUFFER;

    state->totalRead += ssc_block_footer_read(in, &state->lastBlockFooter);

    return SSC_BLOCK_DECODE_STATE_READY;
}

SSC_FORCE_INLINE SSC_BLOCK_DECODE_STATE ssc_block_decode_read_block_mode_marker(ssc_byte_buffer *restrict in, ssc_block_decode_state *restrict state) {
    if (in->position + sizeof(ssc_mode_marker) > in->size)
        return SSC_BLOCK_DECODE_STATE_STALL_ON_INPUT_BUFFER;

    state->totalRead += ssc_block_mode_marker_read(in, &state->lastModeMarker);

    state->process = SSC_BLOCK_DECODE_PROCESS_READ_DATA;

    return SSC_BLOCK_DECODE_STATE_READY;
}

SSC_FORCE_INLINE void ssc_block_decode_update_totals(ssc_byte_buffer *restrict in, ssc_byte_buffer *restrict out, ssc_block_decode_state *restrict state, const uint_fast64_t inPositionBefore, const uint_fast64_t outPositionBefore) {
    state->totalRead += in->position - inPositionBefore;
    state->totalWritten += out->position - outPositionBefore;
}

SSC_FORCE_INLINE SSC_BLOCK_DECODE_STATE ssc_block_decode_init(ssc_block_decode_state *restrict state, const SSC_BLOCK_MODE mode, const SSC_BLOCK_TYPE blockType, const uint_fast32_t endDataOverhead, void *kernelState, SSC_KERNEL_DECODE_STATE (*kernelInit)(void *, const uint32_t), SSC_KERNEL_DECODE_STATE (*kernelProcess)(ssc_byte_buffer *, ssc_byte_buffer *, void *, const ssc_bool), SSC_KERNEL_DECODE_STATE (*kernelFinish)(void *)) {
    state->process = SSC_BLOCK_DECODE_PROCESS_READ_BLOCK_HEADER;
    state->mode = mode;
    state->blockType = blockType;

    state->totalRead = 0;
    state->totalWritten = 0;
    state->endDataOverhead = (state->blockType == SSC_BLOCK_TYPE_DEFAULT ? sizeof(ssc_block_footer) : 0) + endDataOverhead;

    switch (mode) {
        case SSC_BLOCK_MODE_KERNEL:
            state->kernelDecodeState = kernelState;
            state->kernelDecodeInit = kernelInit;
            state->kernelDecodeProcess = kernelProcess;
            state->kernelDecodeFinish = kernelFinish;

            state->kernelDecodeInit(state->kernelDecodeState, state->endDataOverhead);
            break;
        default:
            break;
    }

    return SSC_BLOCK_DECODE_STATE_READY;
}

SSC_FORCE_INLINE SSC_BLOCK_DECODE_STATE ssc_block_decode_process(ssc_byte_buffer *restrict in, ssc_byte_buffer *restrict out, ssc_block_decode_state *restrict state, const ssc_bool flush) {
    SSC_BLOCK_DECODE_STATE decodeState;
    SSC_KERNEL_DECODE_STATE hashDecodeState;
    uint_fast64_t inPositionBefore;
    uint_fast64_t outPositionBefore;
    uint_fast64_t blockRemaining;
    uint_fast64_t inRemaining;
    uint_fast64_t positionIncrement;
    uint_fast64_t outRemaining;

    while (true) {
        switch (state->process) {
            case SSC_BLOCK_DECODE_PROCESS_READ_BLOCK_HEADER:
                if ((decodeState = ssc_block_decode_read_block_header(in, state)))
                    return decodeState;
                break;

            case SSC_BLOCK_DECODE_PROCESS_READ_BLOCK_MODE_MARKER:
                if ((decodeState = ssc_block_decode_read_block_mode_marker(in, state)))
                    return decodeState;
                break;

            case SSC_BLOCK_DECODE_PROCESS_READ_BLOCK_FOOTER:
                if (state->blockType == SSC_BLOCK_TYPE_DEFAULT) if ((decodeState = ssc_block_decode_read_block_footer(in, state)))
                    return decodeState;
                state->process = SSC_BLOCK_DECODE_PROCESS_READ_BLOCK_HEADER;
                break;

            case SSC_BLOCK_DECODE_PROCESS_READ_LAST_BLOCK_FOOTER:
                if (state->blockType == SSC_BLOCK_TYPE_DEFAULT) if ((decodeState = ssc_block_decode_read_block_footer(in, state)))
                    return decodeState;
                state->process = SSC_BLOCK_DECODE_PROCESS_FINISHED;
                return SSC_BLOCK_DECODE_STATE_READY;

            case SSC_BLOCK_DECODE_PROCESS_READ_DATA:
                inPositionBefore = in->position;
                outPositionBefore = out->position;
                switch (state->mode) {
                    case SSC_BLOCK_MODE_COPY:
                        blockRemaining = (uint_fast64_t) SSC_PREFERRED_BUFFER_SIZE - (state->totalWritten - state->currentBlockData.outStart);
                        inRemaining = in->size - in->position;
                        outRemaining = out->size - out->position;

                        if (inRemaining <= outRemaining) {
                            if (blockRemaining <= inRemaining)
                                goto copy_until_end_of_block;
                            else {
                                if (flush && inRemaining <= state->endDataOverhead) {
                                    state->process = SSC_BLOCK_DECODE_PROCESS_READ_LAST_BLOCK_FOOTER;
                                } else if (!inRemaining) {
                                    return SSC_BLOCK_DECODE_STATE_STALL_ON_INPUT_BUFFER;
                                } else {
                                    positionIncrement = inRemaining - (flush ? state->endDataOverhead : 0);
                                    memcpy(out->pointer + out->position, in->pointer + in->position, (size_t) positionIncrement);
                                    in->position += positionIncrement;
                                    out->position += positionIncrement;
                                    ssc_block_decode_update_totals(in, out, state, inPositionBefore, outPositionBefore);
                                }
                            }
                        } else {
                            if (blockRemaining <= outRemaining)
                                goto copy_until_end_of_block;
                            else {
                                if (outRemaining) {
                                    memcpy(out->pointer + out->position, in->pointer + in->position, (size_t) outRemaining);
                                    in->position += outRemaining;
                                    out->position += outRemaining;
                                    ssc_block_decode_update_totals(in, out, state, inPositionBefore, outPositionBefore);
                                } else
                                    return SSC_BLOCK_DECODE_STATE_STALL_ON_OUTPUT_BUFFER;
                            }
                        }
                        goto exit;

                    copy_until_end_of_block:
                        memcpy(out->pointer + out->position, in->pointer + in->position, (size_t) blockRemaining);
                        in->position += blockRemaining;
                        out->position += blockRemaining;
                        ssc_block_decode_update_totals(in, out, state, inPositionBefore, outPositionBefore);
                        state->process = SSC_BLOCK_DECODE_PROCESS_READ_BLOCK_FOOTER;

                    exit:
                        break;

                    case SSC_BLOCK_MODE_KERNEL:
                        hashDecodeState = state->kernelDecodeProcess(in, out, state->kernelDecodeState, flush);
                        ssc_block_decode_update_totals(in, out, state, inPositionBefore, outPositionBefore);

                        switch (hashDecodeState) {
                            case SSC_KERNEL_DECODE_STATE_STALL_ON_INPUT_BUFFER:
                                return SSC_BLOCK_DECODE_STATE_STALL_ON_INPUT_BUFFER;

                            case SSC_KERNEL_DECODE_STATE_STALL_ON_OUTPUT_BUFFER:
                                return SSC_BLOCK_DECODE_STATE_STALL_ON_OUTPUT_BUFFER;

                            case SSC_KERNEL_DECODE_STATE_INFO_NEW_BLOCK:
                                state->process = SSC_BLOCK_DECODE_PROCESS_READ_BLOCK_FOOTER;
                                break;

                            case SSC_KERNEL_DECODE_STATE_INFO_EFFICIENCY_CHECK:
                                state->process = SSC_BLOCK_DECODE_PROCESS_READ_BLOCK_MODE_MARKER;
                                break;

                            case SSC_KERNEL_DECODE_STATE_FINISHED:
                                state->process = SSC_BLOCK_DECODE_PROCESS_READ_LAST_BLOCK_FOOTER;
                                break;

                            case SSC_KERNEL_DECODE_STATE_READY:
                                break;

                            default:
                                return SSC_BLOCK_DECODE_STATE_ERROR;
                        }
                        break;

                    default:
                        return SSC_BLOCK_DECODE_STATE_ERROR;
                }
                break;

            default:
                return SSC_BLOCK_DECODE_STATE_ERROR;
        }
    }
}

SSC_FORCE_INLINE SSC_BLOCK_DECODE_STATE ssc_block_decode_finish(ssc_block_decode_state *restrict state) {
    if (state->process ^ SSC_BLOCK_DECODE_PROCESS_FINISHED)
        return SSC_BLOCK_DECODE_STATE_ERROR;

    state->kernelDecodeFinish(state->kernelDecodeState);

    return SSC_BLOCK_DECODE_STATE_READY;
}