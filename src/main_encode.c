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
 * 18/10/13 23:48
 */

#include "main_encode.h"

SSC_FORCE_INLINE SSC_ENCODE_STATE ssc_encode_write_header(ssc_byte_buffer *restrict out, ssc_encode_state *restrict state, const SSC_COMPRESSION_MODE compressionMode, const SSC_BLOCK_TYPE blockType) {
    if (out->position + sizeof(ssc_main_header) > out->size)
        return SSC_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER;

    state->totalWritten += ssc_main_header_write(out, compressionMode, blockType);

    state->process = SSC_ENCODE_PROCESS_WRITE_BLOCKS;

    return SSC_ENCODE_STATE_READY;
}

SSC_FORCE_INLINE SSC_ENCODE_STATE ssc_encode_write_footer(ssc_byte_buffer *restrict out, ssc_encode_state *restrict state) {
    if (out->position + sizeof(ssc_main_footer) > out->size)
        return SSC_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER;

    state->totalWritten += ssc_main_footer_write(out);

    state->process = SSC_ENCODE_PROCESS_FINISHED;

    return SSC_ENCODE_STATE_READY;
}

SSC_FORCE_INLINE void ssc_encode_update_totals(ssc_byte_buffer *restrict in, ssc_byte_buffer *restrict out, ssc_encode_state *restrict state, const uint_fast64_t inPositionBefore, const uint_fast64_t outPositionBefore) {
    state->totalRead += in->position - inPositionBefore;
    state->totalWritten += out->position - outPositionBefore;
}

SSC_FORCE_INLINE SSC_ENCODE_STATE ssc_encode_init(ssc_byte_buffer *restrict out, ssc_byte_buffer *restrict workBuffer, const uint_fast64_t workBufferSize, ssc_encode_state *restrict state, const SSC_COMPRESSION_MODE mode, const SSC_ENCODE_OUTPUT_TYPE encodeOutputType, const SSC_BLOCK_TYPE blockType) {
    state->compressionMode = mode;
    state->blockType = blockType;
    state->encodeOutputType = encodeOutputType;

    state->totalRead = 0;
    state->totalWritten = 0;

    switch (mode) {
        case SSC_COMPRESSION_MODE_COPY:
            ssc_block_encode_init(&state->blockEncodeStateA, SSC_BLOCK_MODE_COPY, blockType, NULL, NULL, NULL, NULL);
            break;

        case SSC_COMPRESSION_MODE_CHAMELEON:
            ssc_block_encode_init(&state->blockEncodeStateA, SSC_BLOCK_MODE_KERNEL, blockType, malloc(sizeof(ssc_chameleon_encode_state)), (void *) ssc_chameleon_encode_init_1p, (void *) ssc_chameleon_encode_process_1p, (void *) ssc_chameleon_encode_finish_1p);
            break;

        case SSC_COMPRESSION_MODE_DUAL_PASS_CHAMELEON:
            ssc_block_encode_init(&state->blockEncodeStateA, SSC_BLOCK_MODE_KERNEL, SSC_BLOCK_TYPE_NO_HASHSUM_INTEGRITY_CHECK, malloc(sizeof(ssc_chameleon_encode_state)), (void *) ssc_chameleon_encode_init_1p, (void *) ssc_chameleon_encode_process_1p, (void *) ssc_chameleon_encode_finish_1p);
            ssc_block_encode_init(&state->blockEncodeStateB, SSC_BLOCK_MODE_KERNEL, blockType, malloc(sizeof(ssc_chameleon_encode_state)), (void *) ssc_chameleon_encode_init_2p, (void *) ssc_chameleon_encode_process_2p, (void *) ssc_chameleon_encode_finish_2p);
            break;
    }

    state->workBuffer = workBuffer;
    state->workBufferData.memorySize = workBufferSize;

    switch (state->encodeOutputType) {
        case SSC_ENCODE_OUTPUT_TYPE_WITHOUT_HEADER:
        case SSC_ENCODE_OUTPUT_TYPE_WITHOUT_HEADER_NOR_FOOTER:
            state->process = SSC_ENCODE_PROCESS_WRITE_BLOCKS;
            return SSC_ENCODE_STATE_READY;
        default:
            return ssc_encode_write_header(out, state, mode, blockType);
    }
}

SSC_FORCE_INLINE SSC_ENCODE_STATE ssc_encode_process(ssc_byte_buffer *restrict in, ssc_byte_buffer *restrict out, ssc_encode_state *restrict state, const ssc_bool flush) {
    SSC_BLOCK_ENCODE_STATE blockEncodeState;
    uint_fast64_t inPositionBefore;
    uint_fast64_t outPositionBefore;

    while (true) {
        inPositionBefore = in->position;
        outPositionBefore = out->position;

        switch (state->process) {
            case SSC_ENCODE_PROCESS_WRITE_BLOCKS:
                switch (state->compressionMode) {
                    case SSC_COMPRESSION_MODE_COPY:
                    case SSC_COMPRESSION_MODE_CHAMELEON:
                        blockEncodeState = ssc_block_encode_process(in, out, &state->blockEncodeStateA, flush);
                        ssc_encode_update_totals(in, out, state, inPositionBefore, outPositionBefore);

                        switch (blockEncodeState) {
                            case SSC_BLOCK_ENCODE_STATE_READY:
                                state->process = SSC_ENCODE_PROCESS_WRITE_FOOTER;
                                return SSC_ENCODE_STATE_READY;

                            case SSC_BLOCK_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER:
                                return SSC_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER;

                            case SSC_BLOCK_ENCODE_STATE_STALL_ON_INPUT_BUFFER:
                                return SSC_ENCODE_STATE_STALL_ON_INPUT_BUFFER;

                            case SSC_BLOCK_ENCODE_STATE_ERROR:
                                return SSC_ENCODE_STATE_ERROR;
                        }
                        break;

                    case SSC_COMPRESSION_MODE_DUAL_PASS_CHAMELEON:
                        state->process = SSC_ENCODE_PROCESS_WRITE_BLOCKS_IN_TO_WORKBUFFER;
                        break;
                }
                break;

            case SSC_ENCODE_PROCESS_WRITE_BLOCKS_IN_TO_WORKBUFFER:
                state->workBuffer->size = state->workBufferData.memorySize;
                blockEncodeState = ssc_block_encode_process(in, state->workBuffer, &state->blockEncodeStateA, flush);
                state->totalRead += in->position - inPositionBefore;
                switch (blockEncodeState) {
                    case SSC_BLOCK_ENCODE_STATE_READY:
                    case SSC_BLOCK_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER:
                        break;

                    case SSC_BLOCK_ENCODE_STATE_STALL_ON_INPUT_BUFFER:
                        return SSC_ENCODE_STATE_STALL_ON_INPUT_BUFFER;

                    default:
                        return SSC_ENCODE_STATE_ERROR;
                }

                state->workBufferData.outstandingBytes = state->workBuffer->position & 0x1F;
                state->workBuffer->size = flush ? state->workBuffer->position : state->workBuffer->position & ~0x1F;
                ssc_byte_buffer_rewind(state->workBuffer);

                state->process = SSC_ENCODE_PROCESS_WRITE_BLOCKS_WORKBUFFER_TO_OUT;
                break;

            case SSC_ENCODE_PROCESS_WRITE_BLOCKS_WORKBUFFER_TO_OUT:
                blockEncodeState = ssc_block_encode_process(state->workBuffer, out, &state->blockEncodeStateB, flush && in->position == in->size);
                state->totalWritten += out->position - outPositionBefore;
                switch (blockEncodeState) {
                    case SSC_BLOCK_ENCODE_STATE_READY:
                        state->process = SSC_ENCODE_PROCESS_WRITE_FOOTER;
                        return SSC_ENCODE_STATE_READY;

                    case SSC_BLOCK_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER:
                        return SSC_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER;

                    case SSC_BLOCK_ENCODE_STATE_STALL_ON_INPUT_BUFFER:
                        break;

                    default:
                        return SSC_ENCODE_STATE_ERROR;
                }

                if (state->workBuffer->size & ~0x1F)
                memcpy(state->workBuffer->pointer, state->workBuffer->pointer + (state->workBuffer->size & ~0x1F), (size_t) state->workBufferData.outstandingBytes);
                state->workBuffer->position = state->workBufferData.outstandingBytes;

                state->process = SSC_ENCODE_PROCESS_WRITE_BLOCKS_IN_TO_WORKBUFFER;
                break;

            default:
                return SSC_ENCODE_STATE_ERROR;
        }
    }
}

SSC_FORCE_INLINE SSC_ENCODE_STATE ssc_encode_finish(ssc_byte_buffer *restrict out, ssc_encode_state *restrict state) {
    if (state->process ^ SSC_ENCODE_PROCESS_WRITE_FOOTER)
        return SSC_ENCODE_STATE_ERROR;

    switch (state->compressionMode) {
        case SSC_COMPRESSION_MODE_DUAL_PASS_CHAMELEON:
            ssc_block_encode_finish(&state->blockEncodeStateB);
            free(state->blockEncodeStateB.kernelEncodeState);

        case SSC_COMPRESSION_MODE_CHAMELEON:
            ssc_block_encode_finish(&state->blockEncodeStateA);
            free(state->blockEncodeStateA.kernelEncodeState);
            break;

        default:
            break;
    }

    switch (state->encodeOutputType) {
        case SSC_ENCODE_OUTPUT_TYPE_WITHOUT_FOOTER:
        case SSC_ENCODE_OUTPUT_TYPE_WITHOUT_HEADER_NOR_FOOTER:
            return SSC_ENCODE_STATE_READY;
        default:
            return ssc_encode_write_footer(out, state);
    }
}