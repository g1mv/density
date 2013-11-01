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
 * 18/10/13 22:39
 */

#include "buffers.h"

SSC_FORCE_INLINE SSC_BUFFERS_STATE ssc_buffers_translate_state(SSC_STREAM_STATE state) {
    switch (state) {
        case SSC_STREAM_STATE_READY:
            return SSC_BUFFERS_STATE_OK;
        case SSC_STREAM_STATE_STALL_ON_OUTPUT_BUFFER:
            return SSC_BUFFERS_STATE_ERROR_OUTPUT_BUFFER_TOO_SMALL;
        default:
            return SSC_BUFFERS_STATE_ERROR_INVALID_STATE;
    }
}

SSC_FORCE_INLINE SSC_BUFFERS_STATE ssc_buffers_max_compressed_length(uint_fast64_t *result, uint_fast64_t initialLength, const SSC_COMPRESSION_MODE compressionMode) {
    *result = ssc_metadata_max_compressed_length(initialLength, compressionMode, true);

    return SSC_BUFFERS_STATE_OK;
}

SSC_FORCE_INLINE SSC_BUFFERS_STATE ssc_buffers_compress(uint_fast64_t *restrict written, uint8_t *restrict in, uint_fast64_t inSize, uint8_t *restrict out, uint_fast64_t outSize, const SSC_COMPRESSION_MODE compressionMode, const SSC_ENCODE_OUTPUT_TYPE outputType, const SSC_BLOCK_TYPE blockType, void *(*mem_alloc)(size_t), void (*mem_free)(void *)) {
    SSC_STREAM_STATE returnState;

    ssc_stream *stream;
    if (mem_alloc)
        stream = mem_alloc(sizeof(ssc_stream));
    else
        stream = malloc(sizeof(ssc_stream));

    if ((returnState = ssc_stream_prepare(stream, in, inSize, out, outSize, mem_alloc, mem_free)))
        return ssc_buffers_translate_state(returnState);

    if ((returnState = ssc_stream_compress_init(stream, compressionMode, outputType, blockType)))
        return ssc_buffers_translate_state(returnState);

    if ((returnState = ssc_stream_compress(stream, true)))
        return ssc_buffers_translate_state(returnState);

    if ((returnState = ssc_stream_compress_finish(stream)))
        return ssc_buffers_translate_state(returnState);

    if (written)
        *written = *stream->out_total_written;

    if (mem_free)
        mem_free(stream);
    else
        free(stream);

    return SSC_BUFFERS_STATE_OK;
}

SSC_BUFFERS_STATE ssc_buffers_decompress(uint_fast64_t *restrict written, ssc_main_header *restrict header, uint8_t *restrict in, uint_fast64_t inSize, uint8_t *restrict out, uint_fast64_t outSize, void *(*mem_alloc)(size_t), void (*mem_free)(void *)) {
    SSC_STREAM_STATE returnState;

    ssc_stream *stream;
    if (mem_alloc)
        stream = mem_alloc(sizeof(ssc_stream));
    else
        stream = malloc(sizeof(ssc_stream));

    if ((returnState = ssc_stream_prepare(stream, in, inSize, out, outSize, mem_alloc, mem_free)))
        return ssc_buffers_translate_state(returnState);

    if ((returnState = ssc_stream_decompress_init(stream)))
        return ssc_buffers_translate_state(returnState);

    if (header)
        *header = ((ssc_stream_state *) stream->internal_state)->internal_decode_state.header;

    if ((returnState = ssc_stream_decompress(stream, true)))
        return ssc_buffers_translate_state(returnState);

    if ((returnState = ssc_stream_decompress_finish(stream)))
        return ssc_buffers_translate_state(returnState);

    if (written)
        *written = *stream->out_total_written;

    if (mem_free)
        mem_free(stream);
    else
        free(stream);

    return SSC_BUFFERS_STATE_OK;
}