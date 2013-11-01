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
 * 18/10/13 22:36
 */

#include "stream.h"

SSC_FORCE_INLINE SSC_STREAM_STATE ssc_stream_prepare(ssc_stream *restrict stream, uint8_t *restrict in, const uint_fast64_t availableIn, uint8_t *restrict out, const uint_fast64_t availableOut, void *(*mem_alloc)(size_t), void (*mem_free)(void *)) {
    ssc_byte_buffer_encapsulate(&stream->in, in, availableIn);
    ssc_byte_buffer_encapsulate(&stream->out, out, availableOut);

    if (mem_alloc == NULL) {
        stream->internal_state = (ssc_stream_state *) malloc(sizeof(ssc_stream_state));
        ((ssc_stream_state *) stream->internal_state)->mem_alloc = malloc;
    } else {
        stream->internal_state = (ssc_stream_state *) mem_alloc(sizeof(ssc_stream_state));
        ((ssc_stream_state *) stream->internal_state)->mem_alloc = mem_alloc;
    }

    if (mem_free == NULL)
        ((ssc_stream_state *) stream->internal_state)->mem_free = free;
    else
        ((ssc_stream_state *) stream->internal_state)->mem_free = mem_free;

    ((ssc_stream_state *) stream->internal_state)->process = SSC_STREAM_PROCESS_PREPARED;

    return SSC_STREAM_STATE_READY;
}

SSC_FORCE_INLINE SSC_STREAM_STATE ssc_stream_check_conformity(ssc_stream *stream) {
    if (stream->out.size < SSC_STREAM_MINIMUM_OUT_BUFFER_SIZE)
        return SSC_STREAM_STATE_ERROR_OUTPUT_BUFFER_TOO_SMALL;

    return SSC_STREAM_STATE_READY;
}

SSC_FORCE_INLINE void ssc_stream_malloc_work_buffer(ssc_stream *stream, uint_fast64_t workSize) {
    if (((ssc_stream_state *) stream->internal_state)->workBuffer.size ^ workSize) {
        ((ssc_stream_state *) stream->internal_state)->mem_free(((ssc_stream_state *) stream->internal_state)->workBuffer.pointer);
        ssc_byte_buffer_encapsulate(&((ssc_stream_state *) stream->internal_state)->workBuffer, ((ssc_stream_state *) stream->internal_state)->mem_alloc((size_t) workSize), workSize);
    } else
        ssc_byte_buffer_rewind(&((ssc_stream_state *) stream->internal_state)->workBuffer);
}

SSC_FORCE_INLINE void ssc_stream_free_work_buffer(ssc_stream *stream) {
    ((ssc_stream_state *) stream->internal_state)->mem_free(((ssc_stream_state *) stream->internal_state)->workBuffer.pointer);
    ((ssc_stream_state *) stream->internal_state)->workBuffer.pointer = NULL;
}

SSC_FORCE_INLINE SSC_STREAM_STATE ssc_stream_compress_init(ssc_stream *restrict stream, const SSC_COMPRESSION_MODE compressionMode, const SSC_ENCODE_OUTPUT_TYPE outputType, const SSC_BLOCK_TYPE blockType) {
    if (((ssc_stream_state *) stream->internal_state)->process ^ SSC_STREAM_PROCESS_PREPARED)
        return SSC_STREAM_STATE_ERROR_INVALID_INTERNAL_STATE;

    SSC_STREAM_STATE streamState = ssc_stream_check_conformity(stream);
    if (streamState)
        return streamState;

    uint_fast64_t workBufferSize = /* todo SSC_HASH_ENCODE_MINIMUM_OUTPUT_LOOKAHEAD*/ + 0x20 + ssc_metadata_max_compressed_length(stream->in.size, SSC_COMPRESSION_MODE_CHAMELEON, false);
    SSC_ENCODE_STATE encodeState = ssc_encode_init(&stream->out, &((ssc_stream_state *) stream->internal_state)->workBuffer, workBufferSize, &((ssc_stream_state *) stream->internal_state)->internal_encode_state, compressionMode, outputType, blockType);
    switch (encodeState) {
        case SSC_ENCODE_STATE_READY:
            break;

        case SSC_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER:
            return SSC_STREAM_STATE_STALL_ON_OUTPUT_BUFFER;

        default:
            return SSC_STREAM_STATE_ERROR_INVALID_INTERNAL_STATE;
    }

    stream->in_total_read = &((ssc_stream_state *) stream->internal_state)->internal_encode_state.totalRead;
    stream->out_total_written = &((ssc_stream_state *) stream->internal_state)->internal_encode_state.totalWritten;

    if (compressionMode == SSC_COMPRESSION_MODE_DUAL_PASS_CHAMELEON)
        ssc_stream_malloc_work_buffer(stream, workBufferSize);

    ((ssc_stream_state *) stream->internal_state)->process = SSC_STREAM_PROCESS_COMPRESSION_INITED;

    return SSC_STREAM_STATE_READY;
}

SSC_FORCE_INLINE SSC_STREAM_STATE ssc_stream_compress(ssc_stream *stream, const ssc_bool flush) {
    SSC_ENCODE_STATE encodeState;

    if (((ssc_stream_state *) stream->internal_state)->process ^ SSC_STREAM_PROCESS_COMPRESSION_INITED)
        return SSC_STREAM_STATE_ERROR_INVALID_INTERNAL_STATE;

    SSC_STREAM_STATE streamState = ssc_stream_check_conformity(stream);
    if (streamState)
        return streamState;

    if (!flush) if (stream->in.size & 0x1F)
        return SSC_STREAM_STATE_ERROR_INPUT_BUFFER_SIZE_NOT_MULTIPLE_OF_32;

    encodeState = ssc_encode_process(&stream->in, &stream->out, &((ssc_stream_state *) stream->internal_state)->internal_encode_state, flush);
    switch (encodeState) {
        case SSC_ENCODE_STATE_READY:
            break;

        case SSC_ENCODE_STATE_STALL_ON_INPUT_BUFFER:
            return SSC_STREAM_STATE_STALL_ON_INPUT_BUFFER;

        case SSC_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER:
            return SSC_STREAM_STATE_STALL_ON_OUTPUT_BUFFER;

        default:
            return SSC_STREAM_STATE_ERROR_INVALID_INTERNAL_STATE;
    }

    ((ssc_stream_state *) stream->internal_state)->process = SSC_STREAM_PROCESS_COMPRESSION_DATA_FINISHED;

    return SSC_STREAM_STATE_READY;
}

SSC_FORCE_INLINE SSC_STREAM_STATE ssc_stream_compress_finish(ssc_stream *stream) {
    if (((ssc_stream_state *) stream->internal_state)->process ^ SSC_STREAM_PROCESS_COMPRESSION_DATA_FINISHED)
        return SSC_STREAM_STATE_ERROR_INVALID_INTERNAL_STATE;

    SSC_STREAM_STATE streamState = ssc_stream_check_conformity(stream);
    if (streamState)
        return streamState;

    SSC_ENCODE_STATE encodeState = ssc_encode_finish(&stream->out, &((ssc_stream_state *) stream->internal_state)->internal_encode_state);
    switch (encodeState) {
        case SSC_ENCODE_STATE_READY:
            break;

        case SSC_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER:
            return SSC_STREAM_STATE_STALL_ON_OUTPUT_BUFFER;

        default:
            return SSC_STREAM_STATE_ERROR_INVALID_INTERNAL_STATE;
    }

    if (((ssc_stream_state *) stream->internal_state)->internal_encode_state.compressionMode == SSC_COMPRESSION_MODE_DUAL_PASS_CHAMELEON)
        ssc_stream_free_work_buffer(stream);

    ((ssc_stream_state *) stream->internal_state)->process = SSC_STREAM_PROCESS_COMPRESSION_FINISHED;

    return SSC_STREAM_STATE_READY;
}

SSC_FORCE_INLINE SSC_STREAM_STATE ssc_stream_decompress_init(ssc_stream *stream) {
    if (((ssc_stream_state *) stream->internal_state)->process ^ SSC_STREAM_PROCESS_PREPARED)
        return SSC_STREAM_STATE_ERROR_INVALID_INTERNAL_STATE;

    SSC_STREAM_STATE streamState = ssc_stream_check_conformity(stream);
    if (streamState)
        return streamState;

    uint_fast64_t workBufferSize = 0x20 + ssc_metadata_max_decompressed_length(stream->in.size, SSC_COMPRESSION_MODE_CHAMELEON, false);
    SSC_DECODE_STATE decodeState = ssc_decode_init(&stream->in, &((ssc_stream_state *) stream->internal_state)->workBuffer, workBufferSize, &((ssc_stream_state *) stream->internal_state)->internal_decode_state);
    switch (decodeState) {
        case SSC_DECODE_STATE_READY:
            break;

        case SSC_DECODE_STATE_STALL_ON_INPUT_BUFFER:
            return SSC_STREAM_STATE_STALL_ON_INPUT_BUFFER;

        default:
            return SSC_STREAM_STATE_ERROR_INVALID_INTERNAL_STATE;
    }

    stream->in_total_read = &((ssc_stream_state *) stream->internal_state)->internal_decode_state.totalRead;
    stream->out_total_written = &((ssc_stream_state *) stream->internal_state)->internal_decode_state.totalWritten;

    if (((ssc_stream_state *) stream->internal_state)->internal_decode_state.header.compressionMode == SSC_COMPRESSION_MODE_DUAL_PASS_CHAMELEON)
        ssc_stream_malloc_work_buffer(stream, workBufferSize);

    ((ssc_stream_state *) stream->internal_state)->process = SSC_STREAM_PROCESS_DECOMPRESSION_INITED;

    return SSC_STREAM_STATE_READY;
}

SSC_FORCE_INLINE SSC_STREAM_STATE ssc_stream_decompress(ssc_stream *stream, const ssc_bool flush) {
    if (((ssc_stream_state *) stream->internal_state)->process ^ SSC_STREAM_PROCESS_DECOMPRESSION_INITED)
        return SSC_STREAM_STATE_ERROR_INVALID_INTERNAL_STATE;

    SSC_STREAM_STATE streamState = ssc_stream_check_conformity(stream);
    if (streamState)
        return streamState;

    SSC_DECODE_STATE decodeState = ssc_decode_process(&stream->in, &stream->out, &((ssc_stream_state *) stream->internal_state)->internal_decode_state, flush);
    switch (decodeState) {
        case SSC_DECODE_STATE_READY:
            break;

        case SSC_DECODE_STATE_STALL_ON_INPUT_BUFFER:
            return SSC_STREAM_STATE_STALL_ON_INPUT_BUFFER;

        case SSC_DECODE_STATE_STALL_ON_OUTPUT_BUFFER:
            return SSC_STREAM_STATE_STALL_ON_OUTPUT_BUFFER;

        default:
            return SSC_STREAM_STATE_ERROR_INVALID_INTERNAL_STATE;
    }

    ((ssc_stream_state *) stream->internal_state)->process = SSC_STREAM_PROCESS_DECOMPRESSION_DATA_FINISHED;

    return SSC_STREAM_STATE_READY;
}

SSC_FORCE_INLINE SSC_STREAM_STATE ssc_stream_decompress_finish(ssc_stream *stream) {
    if (((ssc_stream_state *) stream->internal_state)->process ^ SSC_STREAM_PROCESS_DECOMPRESSION_DATA_FINISHED)
        return SSC_STREAM_STATE_ERROR_INVALID_INTERNAL_STATE;

    SSC_DECODE_STATE decodeState = ssc_decode_finish(&stream->in, &((ssc_stream_state *) stream->internal_state)->internal_decode_state);
    switch (decodeState) {
        case SSC_DECODE_STATE_READY:
            break;

        case SSC_DECODE_STATE_STALL_ON_INPUT_BUFFER:
            return SSC_STREAM_STATE_STALL_ON_INPUT_BUFFER;

        default:
            return SSC_STREAM_STATE_ERROR_INVALID_INTERNAL_STATE;
    }

    if (((ssc_stream_state *) stream->internal_state)->internal_decode_state.header.compressionMode == SSC_COMPRESSION_MODE_DUAL_PASS_CHAMELEON)
        ssc_stream_free_work_buffer(stream);

    ((ssc_stream_state *) stream->internal_state)->process = SSC_STREAM_PROCESS_DECOMPRESSION_FINISHED;

    return SSC_STREAM_STATE_READY;
}

SSC_FORCE_INLINE SSC_STREAM_STATE ssc_stream_decompress_utilities_get_header(ssc_stream *restrict stream, ssc_main_header *restrict header) {
    switch (((ssc_stream_state *) stream->internal_state)->process) {
        case  SSC_STREAM_PROCESS_DECOMPRESSION_INITED:
        case SSC_STREAM_PROCESS_DECOMPRESSION_DATA_FINISHED:
        case SSC_STREAM_PROCESS_DECOMPRESSION_FINISHED:
            *header = ((ssc_stream_state *) stream->internal_state)->internal_decode_state.header;
            return SSC_STREAM_STATE_READY;
        default:
            return SSC_STREAM_STATE_ERROR_INVALID_INTERNAL_STATE;
    }
}