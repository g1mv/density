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
 * 18/10/13 22:34
 */

#ifndef SSC_API_STREAM_H
#define SSC_API_STREAM_H

#include <stdint.h>

#include "byte_buffer.h"
#include "main_header.h"
#include "block.h"
#include "metadata.h"
#include "globals.h"
#include "main_encode.h"
#include "main_decode.h"

#define SSC_STREAM_MINIMUM_OUT_BUFFER_SIZE                        (1 << 9)

typedef enum {
    SSC_STREAM_PROCESS_PREPARED,
    SSC_STREAM_PROCESS_COMPRESSION_INITED,
    SSC_STREAM_PROCESS_COMPRESSION_DATA_FINISHED,
    SSC_STREAM_PROCESS_COMPRESSION_FINISHED,
    SSC_STREAM_PROCESS_DECOMPRESSION_INITED,
    SSC_STREAM_PROCESS_DECOMPRESSION_DATA_FINISHED,
    SSC_STREAM_PROCESS_DECOMPRESSION_FINISHED,
} SSC_STREAM_PROCESS;

typedef struct {
    SSC_STREAM_PROCESS process;

    ssc_byte_buffer workBuffer;

    void *(*mem_alloc)(size_t);
    void (*mem_free)(void *);

    ssc_encode_state internal_encode_state;
    ssc_decode_state internal_decode_state;
} ssc_stream_state;

SSC_STREAM_STATE ssc_stream_prepare(ssc_stream *, uint8_t*, const uint_fast64_t, uint8_t*, const uint_fast64_t, void *(*)(size_t), void (*)(void *));

SSC_STREAM_STATE ssc_stream_compress_init(ssc_stream *, const SSC_COMPRESSION_MODE, const SSC_ENCODE_OUTPUT_TYPE, const SSC_BLOCK_TYPE);
SSC_STREAM_STATE ssc_stream_compress(ssc_stream *, const ssc_bool);
SSC_STREAM_STATE ssc_stream_compress_finish(ssc_stream *);

SSC_STREAM_STATE ssc_stream_decompress_init(ssc_stream *);
SSC_STREAM_STATE ssc_stream_decompress(ssc_stream *, const ssc_bool);
SSC_STREAM_STATE ssc_stream_decompress_finish(ssc_stream *);

SSC_STREAM_STATE ssc_stream_decompress_utilities_get_header(ssc_stream*, ssc_main_header*);

#endif