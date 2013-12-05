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
 * 18/10/13 22:34
 */

#ifndef DENSITY_API_STREAM_H
#define DENSITY_API_STREAM_H

#include <stdint.h>

#include "byte_buffer.h"
#include "main_header.h"
#include "block.h"
#include "metadata.h"
#include "globals.h"
#include "main_encode.h"
#include "main_decode.h"

#define DENSITY_STREAM_MINIMUM_OUT_BUFFER_SIZE                        (1 << 9)

typedef enum {
    DENSITY_STREAM_PROCESS_PREPARED,
    DENSITY_STREAM_PROCESS_COMPRESSION_INITED,
    DENSITY_STREAM_PROCESS_COMPRESSION_DATA_FINISHED,
    DENSITY_STREAM_PROCESS_COMPRESSION_FINISHED,
    DENSITY_STREAM_PROCESS_DECOMPRESSION_INITED,
    DENSITY_STREAM_PROCESS_DECOMPRESSION_DATA_FINISHED,
    DENSITY_STREAM_PROCESS_DECOMPRESSION_FINISHED,
} DENSITY_STREAM_PROCESS;

typedef struct {
    DENSITY_STREAM_PROCESS process;

    density_byte_buffer workBuffer;

    void *(*mem_alloc)(size_t);
    void (*mem_free)(void *);

    density_encode_state internal_encode_state;
    density_decode_state internal_decode_state;
} density_stream_state;

DENSITY_STREAM_STATE density_stream_prepare(density_stream *, uint8_t*, const uint_fast64_t, uint8_t*, const uint_fast64_t, void *(*)(size_t), void (*)(void *));

DENSITY_STREAM_STATE density_stream_compress_init(density_stream *, const DENSITY_COMPRESSION_MODE, const DENSITY_ENCODE_OUTPUT_TYPE, const DENSITY_BLOCK_TYPE);
DENSITY_STREAM_STATE density_stream_compress(density_stream *, const density_bool);
DENSITY_STREAM_STATE density_stream_compress_finish(density_stream *);

DENSITY_STREAM_STATE density_stream_decompress_init(density_stream *);
DENSITY_STREAM_STATE density_stream_decompress(density_stream *, const density_bool);
DENSITY_STREAM_STATE density_stream_decompress_finish(density_stream *);

DENSITY_STREAM_STATE density_stream_decompress_utilities_get_header(density_stream*, density_main_header*);

#endif