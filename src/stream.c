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
 * 18/10/13 22:36
 */

#include "stream.h"

DENSITY_FORCE_INLINE DENSITY_STREAM_STATE density_stream_prepare(density_stream *restrict stream, uint8_t *restrict in, const uint_fast64_t availableIn, uint8_t *restrict out, const uint_fast64_t availableOut, void *(*mem_alloc)(size_t), void (*mem_free)(void *)) {
    density_memory_location_encapsulate(&stream->in, in, availableIn);
    density_memory_location_encapsulate(&stream->out, out, availableOut);

    if (mem_alloc == NULL) {
        stream->internal_state = (density_stream_state *) malloc(sizeof(density_stream_state));
        ((density_stream_state *) stream->internal_state)->mem_alloc = malloc;
    } else {
        stream->internal_state = (density_stream_state *) mem_alloc(sizeof(density_stream_state));
        ((density_stream_state *) stream->internal_state)->mem_alloc = mem_alloc;
    }

    if (mem_free == NULL)
        ((density_stream_state *) stream->internal_state)->mem_free = free;
    else
        ((density_stream_state *) stream->internal_state)->mem_free = mem_free;

    ((density_stream_state *) stream->internal_state)->process = DENSITY_STREAM_PROCESS_PREPARED;

    return DENSITY_STREAM_STATE_READY;
}

DENSITY_FORCE_INLINE DENSITY_STREAM_STATE density_stream_check_conformity(density_stream *stream) {
    if (stream->out.available_bytes < DENSITY_STREAM_MINIMUM_OUT_BUFFER_SIZE)
        return DENSITY_STREAM_STATE_ERROR_OUTPUT_BUFFER_TOO_SMALL;

    return DENSITY_STREAM_STATE_READY;
}

DENSITY_FORCE_INLINE DENSITY_STREAM_STATE density_stream_compress_init(density_stream *restrict stream, const DENSITY_COMPRESSION_MODE compressionMode, const DENSITY_ENCODE_OUTPUT_TYPE outputType, const DENSITY_BLOCK_TYPE blockType) {
    if (((density_stream_state *) stream->internal_state)->process ^ DENSITY_STREAM_PROCESS_PREPARED)
        return DENSITY_STREAM_STATE_ERROR_INVALID_INTERNAL_STATE;

    DENSITY_STREAM_STATE streamState = density_stream_check_conformity(stream);
    if (streamState)
        return streamState;

    DENSITY_ENCODE_STATE encodeState = density_encode_init(&stream->out, &((density_stream_state *) stream->internal_state)->internal_encode_state, compressionMode, outputType, blockType);
    switch (encodeState) {
        case DENSITY_ENCODE_STATE_READY:
            break;

        case DENSITY_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER:
            return DENSITY_STREAM_STATE_STALL_ON_OUTPUT_BUFFER;

        default:
            return DENSITY_STREAM_STATE_ERROR_INVALID_INTERNAL_STATE;
    }

    stream->in_total_read = &((density_stream_state *) stream->internal_state)->internal_encode_state.totalRead;
    stream->out_total_written = &((density_stream_state *) stream->internal_state)->internal_encode_state.totalWritten;

    ((density_stream_state *) stream->internal_state)->process = DENSITY_STREAM_PROCESS_COMPRESSION_INITED;

    return DENSITY_STREAM_STATE_READY;
}

DENSITY_FORCE_INLINE DENSITY_STREAM_STATE density_stream_compress(density_stream *stream, const density_bool flush) {
    DENSITY_ENCODE_STATE encodeState;

    if (((density_stream_state *) stream->internal_state)->process ^ DENSITY_STREAM_PROCESS_COMPRESSION_INITED)
        return DENSITY_STREAM_STATE_ERROR_INVALID_INTERNAL_STATE;

    DENSITY_STREAM_STATE streamState = density_stream_check_conformity(stream);
    if (streamState)
        return streamState;

    /*if (!flush) if (stream->in.available_bytes & 0x1F)
        return DENSITY_STREAM_STATE_ERROR_INPUT_BUFFER_SIZE_NOT_MULTIPLE_OF_32;*/

    encodeState = density_encode_process(&stream->in, &stream->out, &((density_stream_state *) stream->internal_state)->internal_encode_state, flush);
    switch (encodeState) {
        case DENSITY_ENCODE_STATE_READY:
            break;

        case DENSITY_ENCODE_STATE_STALL_ON_INPUT_BUFFER:
            return DENSITY_STREAM_STATE_STALL_ON_INPUT_BUFFER;

        case DENSITY_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER:
            return DENSITY_STREAM_STATE_STALL_ON_OUTPUT_BUFFER;

        default:
            return DENSITY_STREAM_STATE_ERROR_INVALID_INTERNAL_STATE;
    }

    ((density_stream_state *) stream->internal_state)->process = DENSITY_STREAM_PROCESS_COMPRESSION_DATA_FINISHED;

    return DENSITY_STREAM_STATE_READY;
}

DENSITY_FORCE_INLINE DENSITY_STREAM_STATE density_stream_compress_finish(density_stream *stream) {
    if (((density_stream_state *) stream->internal_state)->process ^ DENSITY_STREAM_PROCESS_COMPRESSION_DATA_FINISHED)
        return DENSITY_STREAM_STATE_ERROR_INVALID_INTERNAL_STATE;

    DENSITY_STREAM_STATE streamState = density_stream_check_conformity(stream);
    if (streamState)
        return streamState;

    DENSITY_ENCODE_STATE encodeState = density_encode_finish(&stream->out, &((density_stream_state *) stream->internal_state)->internal_encode_state);
    switch (encodeState) {
        case DENSITY_ENCODE_STATE_READY:
            break;

        case DENSITY_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER:
            return DENSITY_STREAM_STATE_STALL_ON_OUTPUT_BUFFER;

        default:
            return DENSITY_STREAM_STATE_ERROR_INVALID_INTERNAL_STATE;
    }

    ((density_stream_state *) stream->internal_state)->process = DENSITY_STREAM_PROCESS_COMPRESSION_FINISHED;

    return DENSITY_STREAM_STATE_READY;
}

DENSITY_FORCE_INLINE DENSITY_STREAM_STATE density_stream_decompress_init(density_stream *stream) {
    if (((density_stream_state *) stream->internal_state)->process ^ DENSITY_STREAM_PROCESS_PREPARED)
        return DENSITY_STREAM_STATE_ERROR_INVALID_INTERNAL_STATE;

    DENSITY_STREAM_STATE streamState = density_stream_check_conformity(stream);
    if (streamState)
        return streamState;

    DENSITY_DECODE_STATE decodeState = density_decode_init(&stream->in, &((density_stream_state *) stream->internal_state)->internal_decode_state);
    switch (decodeState) {
        case DENSITY_DECODE_STATE_READY:
            break;

        case DENSITY_DECODE_STATE_STALL_ON_INPUT_BUFFER:
            return DENSITY_STREAM_STATE_STALL_ON_INPUT_BUFFER;

        default:
            return DENSITY_STREAM_STATE_ERROR_INVALID_INTERNAL_STATE;
    }

    stream->in_total_read = &((density_stream_state *) stream->internal_state)->internal_decode_state.totalRead;
    stream->out_total_written = &((density_stream_state *) stream->internal_state)->internal_decode_state.totalWritten;

    ((density_stream_state *) stream->internal_state)->process = DENSITY_STREAM_PROCESS_DECOMPRESSION_INITED;

    return DENSITY_STREAM_STATE_READY;
}

DENSITY_FORCE_INLINE DENSITY_STREAM_STATE density_stream_decompress(density_stream *stream, const density_bool flush) {
    if (((density_stream_state *) stream->internal_state)->process ^ DENSITY_STREAM_PROCESS_DECOMPRESSION_INITED)
        return DENSITY_STREAM_STATE_ERROR_INVALID_INTERNAL_STATE;

    DENSITY_STREAM_STATE streamState = density_stream_check_conformity(stream);
    if (streamState)
        return streamState;

    DENSITY_DECODE_STATE decodeState = density_decode_process(&stream->in, &stream->out, &((density_stream_state *) stream->internal_state)->internal_decode_state, flush);
    switch (decodeState) {
        case DENSITY_DECODE_STATE_READY:
            break;

        case DENSITY_DECODE_STATE_STALL_ON_INPUT_BUFFER:
            return DENSITY_STREAM_STATE_STALL_ON_INPUT_BUFFER;

        case DENSITY_DECODE_STATE_STALL_ON_OUTPUT_BUFFER:
            return DENSITY_STREAM_STATE_STALL_ON_OUTPUT_BUFFER;

        default:
            return DENSITY_STREAM_STATE_ERROR_INVALID_INTERNAL_STATE;
    }

    ((density_stream_state *) stream->internal_state)->process = DENSITY_STREAM_PROCESS_DECOMPRESSION_DATA_FINISHED;

    return DENSITY_STREAM_STATE_READY;
}

DENSITY_FORCE_INLINE DENSITY_STREAM_STATE density_stream_decompress_finish(density_stream *stream) {
    if (((density_stream_state *) stream->internal_state)->process ^ DENSITY_STREAM_PROCESS_DECOMPRESSION_DATA_FINISHED)
        return DENSITY_STREAM_STATE_ERROR_INVALID_INTERNAL_STATE;

    DENSITY_DECODE_STATE decodeState = density_decode_finish(&stream->in, &((density_stream_state *) stream->internal_state)->internal_decode_state);
    switch (decodeState) {
        case DENSITY_DECODE_STATE_READY:
            break;

        case DENSITY_DECODE_STATE_STALL_ON_INPUT_BUFFER:
            return DENSITY_STREAM_STATE_STALL_ON_INPUT_BUFFER;

        default:
            return DENSITY_STREAM_STATE_ERROR_INVALID_INTERNAL_STATE;
    }

    ((density_stream_state *) stream->internal_state)->process = DENSITY_STREAM_PROCESS_DECOMPRESSION_FINISHED;

    return DENSITY_STREAM_STATE_READY;
}

DENSITY_FORCE_INLINE DENSITY_STREAM_STATE density_stream_decompress_utilities_get_header(density_stream *restrict stream, density_main_header *restrict header) {
    switch (((density_stream_state *) stream->internal_state)->process) {
        case  DENSITY_STREAM_PROCESS_DECOMPRESSION_INITED:
        case DENSITY_STREAM_PROCESS_DECOMPRESSION_DATA_FINISHED:
        case DENSITY_STREAM_PROCESS_DECOMPRESSION_FINISHED:
            *header = ((density_stream_state *) stream->internal_state)->internal_decode_state.header;
            return DENSITY_STREAM_STATE_READY;
        default:
            return DENSITY_STREAM_STATE_ERROR_INVALID_INTERNAL_STATE;
    }
}