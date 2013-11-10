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
 * 19/10/13 00:06
 */

#include "main_decode.h"

SSC_FORCE_INLINE SSC_DECODE_STATE ssc_decode_read_header(ssc_byte_buffer *restrict in, ssc_decode_state *restrict state) {
    if (in->position + sizeof(ssc_main_header) > in->size)
        return SSC_DECODE_STATE_STALL_ON_INPUT_BUFFER;

    state->totalRead += ssc_main_header_read(in, &state->header);

    state->process = SSC_DECODE_PROCESS_READ_BLOCKS;

    return SSC_DECODE_STATE_READY;
}

SSC_FORCE_INLINE SSC_DECODE_STATE ssc_decode_read_footer(ssc_byte_buffer *restrict in, ssc_decode_state *restrict state) {
    if (in->position + sizeof(ssc_main_footer) > in->size)
        return SSC_DECODE_STATE_STALL_ON_INPUT_BUFFER;

    state->totalRead += ssc_main_footer_read(in, &state->footer);

    state->process = SSC_DECODE_PROCESS_FINISHED;

    return SSC_DECODE_STATE_READY;
}

SSC_FORCE_INLINE void ssc_decode_update_totals(ssc_byte_buffer *restrict in, ssc_byte_buffer *restrict out, ssc_decode_state *restrict state, const uint_fast64_t inPositionBefore, const uint_fast64_t outPositionBefore) {
    state->totalRead += in->position - inPositionBefore;
    state->totalWritten += out->position - outPositionBefore;
}

SSC_FORCE_INLINE SSC_DECODE_STATE ssc_decode_init(ssc_byte_buffer *in, ssc_byte_buffer *workBuffer, const uint_fast64_t workBufferSize, ssc_decode_state *restrict state) {
    SSC_DECODE_STATE decodeState;

    state->totalRead = 0;
    state->totalWritten = 0;

    if ((decodeState = ssc_decode_read_header(in, state)))
        return decodeState;

    switch (state->header.compressionMode) {
        case SSC_COMPRESSION_MODE_COPY:
            ssc_block_decode_init(&state->blockDecodeStateA, SSC_BLOCK_MODE_COPY, (SSC_BLOCK_TYPE) state->header.blockType, sizeof(ssc_main_footer), NULL, NULL, NULL, NULL);
            break;

        case SSC_COMPRESSION_MODE_CHAMELEON:
            ssc_block_decode_init(&state->blockDecodeStateA, SSC_BLOCK_MODE_KERNEL, (SSC_BLOCK_TYPE) state->header.blockType, sizeof(ssc_main_footer), malloc(sizeof(ssc_chameleon_decode_state)), (void *) ssc_chameleon_decode_init_1p, (void *) ssc_chameleon_decode_process_1p, (void *) ssc_chameleon_decode_finish_1p);
            break;

        case SSC_COMPRESSION_MODE_DUAL_PASS_CHAMELEON:
            ssc_block_decode_init(&state->blockDecodeStateA, SSC_BLOCK_MODE_KERNEL, (SSC_BLOCK_TYPE) state->header.blockType, sizeof(ssc_main_footer), malloc(sizeof(ssc_chameleon_decode_state)), (void *) ssc_chameleon_decode_init_2p, (void *) ssc_chameleon_decode_process_2p, (void *) ssc_chameleon_decode_finish_2p);
            ssc_block_decode_init(&state->blockDecodeStateB, SSC_BLOCK_MODE_KERNEL, SSC_BLOCK_TYPE_NO_HASHSUM_INTEGRITY_CHECK, 0, malloc(sizeof(ssc_chameleon_decode_state)), (void *) ssc_chameleon_decode_init_1p, (void *) ssc_chameleon_decode_process_1p, (void *) ssc_chameleon_decode_finish_1p);
            break;

        default:
            return SSC_DECODE_STATE_ERROR;
    }

    state->workBuffer = workBuffer;
    state->workBufferData.memorySize = workBufferSize;

    return SSC_DECODE_STATE_READY;
}

SSC_FORCE_INLINE SSC_DECODE_STATE ssc_decode_process(ssc_byte_buffer *restrict in, ssc_byte_buffer *restrict out, ssc_decode_state *restrict state, const ssc_bool flush) {
    SSC_BLOCK_DECODE_STATE blockDecodeState;
    uint_fast64_t inPositionBefore;
    uint_fast64_t outPositionBefore;

    while (true) {
        inPositionBefore = in->position;
        outPositionBefore = out->position;

        switch (state->process) {
            case SSC_DECODE_PROCESS_READ_BLOCKS:
                switch (state->header.compressionMode) {
                    case SSC_COMPRESSION_MODE_COPY:
                    case SSC_COMPRESSION_MODE_CHAMELEON:
                        blockDecodeState = ssc_block_decode_process(in, out, &state->blockDecodeStateA, flush);
                        ssc_decode_update_totals(in, out, state, inPositionBefore, outPositionBefore);

                        switch (blockDecodeState) {
                            case SSC_BLOCK_DECODE_STATE_READY:
                                state->process = SSC_DECODE_PROCESS_READ_FOOTER;
                                return SSC_DECODE_STATE_READY;

                            case SSC_BLOCK_DECODE_STATE_STALL_ON_OUTPUT_BUFFER:
                                return SSC_DECODE_STATE_STALL_ON_OUTPUT_BUFFER;

                            case SSC_BLOCK_DECODE_STATE_STALL_ON_INPUT_BUFFER:
                                return SSC_DECODE_STATE_STALL_ON_INPUT_BUFFER;

                            case SSC_BLOCK_DECODE_STATE_ERROR:
                                return SSC_DECODE_STATE_ERROR;
                        }
                        break;

                    case SSC_COMPRESSION_MODE_DUAL_PASS_CHAMELEON:
                        state->process = SSC_DECODE_PROCESS_READ_BLOCKS_IN_TO_WORKBUFFER;
                        break;

                    default:
                        return SSC_DECODE_STATE_ERROR;
                }
                break;

            case SSC_DECODE_PROCESS_READ_BLOCKS_IN_TO_WORKBUFFER:
                state->workBuffer->size = state->workBufferData.memorySize;
                blockDecodeState = ssc_block_decode_process(in, state->workBuffer, &state->blockDecodeStateA, flush);
                state->totalRead += in->position - inPositionBefore;
                switch (blockDecodeState) {
                    case SSC_BLOCK_DECODE_STATE_READY:
                    case SSC_BLOCK_DECODE_STATE_STALL_ON_OUTPUT_BUFFER:
                        break;

                    case SSC_BLOCK_DECODE_STATE_STALL_ON_INPUT_BUFFER:
                        return SSC_DECODE_STATE_STALL_ON_INPUT_BUFFER;

                    default:
                        return SSC_DECODE_STATE_ERROR;
                }

                state->workBuffer->size = state->workBuffer->position;
                ssc_byte_buffer_rewind(state->workBuffer);

                state->process = SSC_DECODE_PROCESS_READ_BLOCKS_WORKBUFFER_TO_OUT;
                break;

            case SSC_DECODE_PROCESS_READ_BLOCKS_WORKBUFFER_TO_OUT:
                blockDecodeState = ssc_block_decode_process(state->workBuffer, out, &state->blockDecodeStateB, flush && in->position == in->size);
                state->totalWritten += out->position - outPositionBefore;
                switch (blockDecodeState) {
                    case SSC_BLOCK_DECODE_STATE_READY:
                        state->process = SSC_DECODE_PROCESS_READ_FOOTER;
                        return SSC_DECODE_STATE_READY;

                    case SSC_BLOCK_DECODE_STATE_STALL_ON_OUTPUT_BUFFER:
                        return SSC_DECODE_STATE_STALL_ON_OUTPUT_BUFFER;

                    case SSC_BLOCK_DECODE_STATE_STALL_ON_INPUT_BUFFER:
                        break;

                    default:
                        return SSC_DECODE_STATE_ERROR;
                }
                ssc_byte_buffer_rewind(state->workBuffer);

                state->process = SSC_DECODE_PROCESS_READ_BLOCKS_IN_TO_WORKBUFFER;
                break;

            default:
                return SSC_DECODE_STATE_ERROR;
        }
    }
}

SSC_FORCE_INLINE SSC_DECODE_STATE ssc_decode_finish(ssc_byte_buffer *in, ssc_decode_state *restrict state) {
    if (state->process ^ SSC_DECODE_PROCESS_READ_FOOTER)
        return SSC_DECODE_STATE_ERROR;

    switch (state->header.compressionMode) {
        case SSC_COMPRESSION_MODE_DUAL_PASS_CHAMELEON:
            ssc_block_decode_finish(&state->blockDecodeStateB);
            free(state->blockDecodeStateB.kernelDecodeState);

        case SSC_COMPRESSION_MODE_CHAMELEON:
            ssc_block_decode_finish(&state->blockDecodeStateA);
            free(state->blockDecodeStateA.kernelDecodeState);
            break;

        default:
            break;
    }

    return ssc_decode_read_footer(in, state);
}