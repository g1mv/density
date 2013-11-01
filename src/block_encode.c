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
 * 18/10/13 00:03
 */

#include "block_encode.h"

SSC_FORCE_INLINE SSC_BLOCK_ENCODE_STATE ssc_block_encode_write_block_header(ssc_byte_buffer *restrict out, ssc_block_encode_state *restrict state) {
    if (out->position + sizeof(ssc_block_header) > out->size)
        return SSC_BLOCK_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER;

    state->currentMode = state->targetMode;

    state->currentBlockData.inStart = state->totalRead;
    state->currentBlockData.outStart = state->totalWritten;

    state->totalWritten += ssc_block_header_write(out);

    state->process = SSC_BLOCK_ENCODE_PROCESS_WRITE_DATA;

    return SSC_BLOCK_ENCODE_STATE_READY;
}

SSC_FORCE_INLINE SSC_BLOCK_ENCODE_STATE ssc_block_encode_write_block_footer(ssc_byte_buffer *restrict out, ssc_block_encode_state *restrict state) {
    if (out->position + sizeof(ssc_block_footer) > out->size)
        return SSC_BLOCK_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER;

    state->totalWritten += ssc_block_footer_write(out, 0);

    return SSC_BLOCK_ENCODE_STATE_READY;
}

SSC_FORCE_INLINE SSC_BLOCK_ENCODE_STATE ssc_block_encode_write_mode_marker(ssc_byte_buffer *restrict out, ssc_block_encode_state *restrict state) {
    if (out->position + sizeof(ssc_mode_marker) > out->size)
        return SSC_BLOCK_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER;

    state->totalWritten += ssc_block_mode_marker_write(out, state->targetMode);

    state->process = SSC_BLOCK_ENCODE_PROCESS_WRITE_DATA;

    return SSC_BLOCK_ENCODE_STATE_READY;
}

SSC_FORCE_INLINE void ssc_block_encode_update_totals(ssc_byte_buffer *restrict in, ssc_byte_buffer *restrict out, ssc_block_encode_state *restrict state, const uint_fast64_t inPositionBefore, const uint_fast64_t outPositionBefore) {
    state->totalRead += in->position - inPositionBefore;
    state->totalWritten += out->position - outPositionBefore;
}

SSC_FORCE_INLINE SSC_BLOCK_ENCODE_STATE ssc_block_encode_init(ssc_block_encode_state *restrict state, const SSC_BLOCK_MODE mode, const SSC_BLOCK_TYPE blockType, void *kernelState, SSC_KERNEL_ENCODE_STATE (*kernelInit)(void *), SSC_KERNEL_ENCODE_STATE (*kernelProcess)(ssc_byte_buffer *, ssc_byte_buffer *, void *, const ssc_bool), SSC_KERNEL_ENCODE_STATE (*kernelFinish)(void *)) {
    state->process = SSC_BLOCK_ENCODE_PROCESS_WRITE_BLOCK_HEADER;
    state->blockType = blockType;
    state->targetMode = mode;
    state->currentMode = mode;

    state->totalRead = 0;
    state->totalWritten = 0;

    switch (mode) {
        case SSC_BLOCK_MODE_KERNEL:
            state->kernelEncodeState = kernelState;
            state->kernelEncodeInit = kernelInit;
            state->kernelEncodeProcess = kernelProcess;
            state->kernelEncodeFinish = kernelFinish;

            state->kernelEncodeInit(state->kernelEncodeState);
            break;
        default:
            break;
    }

    return SSC_BLOCK_ENCODE_STATE_READY;
}

SSC_FORCE_INLINE SSC_BLOCK_ENCODE_STATE ssc_block_encode_process(ssc_byte_buffer *restrict in, ssc_byte_buffer *restrict out, ssc_block_encode_state *restrict state, const ssc_bool flush) {
    SSC_BLOCK_ENCODE_STATE encodeState;
    SSC_KERNEL_ENCODE_STATE kernelEncodeState;
    uint_fast64_t inPositionBefore;
    uint_fast64_t outPositionBefore;
    uint_fast64_t blockRemaining;
    uint_fast64_t inRemaining;
    uint_fast64_t outRemaining;

    while (true) {
        switch (state->process) {
            case SSC_BLOCK_ENCODE_PROCESS_WRITE_BLOCK_HEADER:
                if ((encodeState = ssc_block_encode_write_block_header(out, state)))
                    return encodeState;
                break;

            case SSC_BLOCK_ENCODE_PROCESS_WRITE_BLOCK_FOOTER:
                if (state->blockType == SSC_BLOCK_TYPE_DEFAULT) if ((encodeState = ssc_block_encode_write_block_footer(out, state)))
                    return encodeState;
                state->process = SSC_BLOCK_ENCODE_PROCESS_WRITE_BLOCK_HEADER;
                break;

            case SSC_BLOCK_ENCODE_PROCESS_WRITE_LAST_BLOCK_FOOTER:
                if (state->blockType == SSC_BLOCK_TYPE_DEFAULT) if ((encodeState = ssc_block_encode_write_block_footer(out, state)))
                    return encodeState;
                state->process = SSC_BLOCK_ENCODE_PROCESS_FINISHED;
                return SSC_BLOCK_ENCODE_STATE_READY;

            case SSC_BLOCK_ENCODE_PROCESS_WRITE_BLOCK_MODE_MARKER:
                if ((encodeState = ssc_block_encode_write_mode_marker(out, state)))
                    return encodeState;
                break;

            case SSC_BLOCK_ENCODE_PROCESS_WRITE_DATA:
                inPositionBefore = in->position;
                outPositionBefore = out->position;

                switch (state->currentMode) {
                    case SSC_BLOCK_MODE_COPY:
                        blockRemaining = (uint_fast64_t)SSC_PREFERRED_BUFFER_SIZE - (state->totalRead - state->currentBlockData.inStart);
                        inRemaining = in->size - in->position;
                        outRemaining = out->size - out->position;

                        if (inRemaining <= outRemaining) {
                            if (blockRemaining <= inRemaining)
                                goto copy_until_end_of_block;
                            else {
                                memcpy(out->pointer + out->position, in->pointer + in->position, (size_t) inRemaining);
                                in->position += inRemaining;
                                out->position += inRemaining;
                                ssc_block_encode_update_totals(in, out, state, inPositionBefore, outPositionBefore);
                                if (flush)
                                    state->process = SSC_BLOCK_ENCODE_PROCESS_WRITE_LAST_BLOCK_FOOTER;
                                else
                                    return SSC_BLOCK_ENCODE_STATE_STALL_ON_INPUT_BUFFER;
                            }
                        } else {
                            if (blockRemaining <= outRemaining)
                                goto copy_until_end_of_block;
                            else {
                                memcpy(out->pointer + out->position, in->pointer + in->position, (size_t) outRemaining);
                                in->position += outRemaining;
                                out->position += outRemaining;
                                ssc_block_encode_update_totals(in, out, state, inPositionBefore, outPositionBefore);
                                return SSC_BLOCK_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER;
                            }
                        }
                        goto exit;

                    copy_until_end_of_block:
                        memcpy(out->pointer + out->position, in->pointer + in->position, (size_t) blockRemaining);
                        in->position += blockRemaining;
                        out->position += blockRemaining;
                        ssc_block_encode_update_totals(in, out, state, inPositionBefore, outPositionBefore);
                        if (flush && inRemaining == blockRemaining)
                            state->process = SSC_BLOCK_ENCODE_PROCESS_WRITE_LAST_BLOCK_FOOTER;
                        else
                            state->process = SSC_BLOCK_ENCODE_PROCESS_WRITE_BLOCK_FOOTER;

                    exit:
                        break;

                    case SSC_BLOCK_MODE_KERNEL:
                        kernelEncodeState = state->kernelEncodeProcess(in, out, state->kernelEncodeState, flush);
                        ssc_block_encode_update_totals(in, out, state, inPositionBefore, outPositionBefore);

                        switch (kernelEncodeState) {
                            case SSC_KERNEL_ENCODE_STATE_STALL_ON_INPUT_BUFFER:
                                return SSC_BLOCK_ENCODE_STATE_STALL_ON_INPUT_BUFFER;

                            case SSC_KERNEL_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER:
                                return SSC_BLOCK_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER;

                            case SSC_KERNEL_ENCODE_STATE_INFO_NEW_BLOCK:
                                state->process = SSC_BLOCK_ENCODE_PROCESS_WRITE_BLOCK_FOOTER;
                                break;

                            case SSC_KERNEL_ENCODE_STATE_INFO_EFFICIENCY_CHECK:
                                state->process = SSC_BLOCK_ENCODE_PROCESS_WRITE_BLOCK_MODE_MARKER;
                                break;

                            case SSC_KERNEL_ENCODE_STATE_FINISHED:
                                state->process = SSC_BLOCK_ENCODE_PROCESS_WRITE_LAST_BLOCK_FOOTER;
                                break;

                            case SSC_KERNEL_ENCODE_STATE_READY:
                                break;

                            default:
                                return SSC_BLOCK_ENCODE_STATE_ERROR;
                        }
                        break;

                    default:
                        return SSC_BLOCK_ENCODE_STATE_ERROR;
                }
                break;

            default:
                return SSC_BLOCK_ENCODE_STATE_ERROR;
        }
    }
}

SSC_FORCE_INLINE SSC_BLOCK_ENCODE_STATE ssc_block_encode_finish(ssc_block_encode_state *restrict state) {
    if (state->process ^ SSC_BLOCK_ENCODE_PROCESS_FINISHED)
        return SSC_BLOCK_ENCODE_STATE_ERROR;

    state->kernelEncodeFinish(state->kernelEncodeState);

    return SSC_BLOCK_ENCODE_STATE_READY;
}