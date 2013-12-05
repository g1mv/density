/*
 * Centaurean Density
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

DENSITY_FORCE_INLINE DENSITY_BUFFERS_STATE density_buffers_translate_state(DENSITY_STREAM_STATE state) {
    switch (state) {
        case DENSITY_STREAM_STATE_READY:
            return DENSITY_BUFFERS_STATE_OK;
        case DENSITY_STREAM_STATE_STALL_ON_OUTPUT_BUFFER:
            return DENSITY_BUFFERS_STATE_ERROR_OUTPUT_BUFFER_TOO_SMALL;
        default:
            return DENSITY_BUFFERS_STATE_ERROR_INVALID_STATE;
    }
}

DENSITY_FORCE_INLINE DENSITY_BUFFERS_STATE density_buffers_max_compressed_length(uint_fast64_t *result, uint_fast64_t initialLength, const DENSITY_COMPRESSION_MODE compressionMode) {
    *result = density_metadata_max_compressed_length(initialLength, compressionMode, true);

    return DENSITY_BUFFERS_STATE_OK;
}

DENSITY_FORCE_INLINE DENSITY_BUFFERS_STATE density_buffers_compress(uint_fast64_t *restrict written, uint8_t *restrict in, uint_fast64_t inSize, uint8_t *restrict out, uint_fast64_t outSize, const DENSITY_COMPRESSION_MODE compressionMode, const DENSITY_ENCODE_OUTPUT_TYPE outputType, const DENSITY_BLOCK_TYPE blockType, void *(*mem_alloc)(size_t), void (*mem_free)(void *)) {
    DENSITY_STREAM_STATE returnState;

    density_stream *stream;
    if (mem_alloc)
        stream = mem_alloc(sizeof(density_stream));
    else
        stream = malloc(sizeof(density_stream));

    if ((returnState = density_stream_prepare(stream, in, inSize, out, outSize, mem_alloc, mem_free)))
        return density_buffers_translate_state(returnState);

    if ((returnState = density_stream_compress_init(stream, compressionMode, outputType, blockType)))
        return density_buffers_translate_state(returnState);

    if ((returnState = density_stream_compress(stream, true)))
        return density_buffers_translate_state(returnState);

    if ((returnState = density_stream_compress_finish(stream)))
        return density_buffers_translate_state(returnState);

    if (written)
        *written = *stream->out_total_written;

    if (mem_free)
        mem_free(stream);
    else
        free(stream);

    return DENSITY_BUFFERS_STATE_OK;
}

DENSITY_BUFFERS_STATE density_buffers_decompress(uint_fast64_t *restrict written, density_main_header *restrict header, uint8_t *restrict in, uint_fast64_t inSize, uint8_t *restrict out, uint_fast64_t outSize, void *(*mem_alloc)(size_t), void (*mem_free)(void *)) {
    DENSITY_STREAM_STATE returnState;

    density_stream *stream;
    if (mem_alloc)
        stream = mem_alloc(sizeof(density_stream));
    else
        stream = malloc(sizeof(density_stream));

    if ((returnState = density_stream_prepare(stream, in, inSize, out, outSize, mem_alloc, mem_free)))
        return density_buffers_translate_state(returnState);

    if ((returnState = density_stream_decompress_init(stream)))
        return density_buffers_translate_state(returnState);

    if (header)
        *header = ((density_stream_state *) stream->internal_state)->internal_decode_state.header;

    if ((returnState = density_stream_decompress(stream, true)))
        return density_buffers_translate_state(returnState);

    if ((returnState = density_stream_decompress_finish(stream)))
        return density_buffers_translate_state(returnState);

    if (written)
        *written = *stream->out_total_written;

    if (mem_free)
        mem_free(stream);
    else
        free(stream);

    return DENSITY_BUFFERS_STATE_OK;
}