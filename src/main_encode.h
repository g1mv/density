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
 * 18/10/13 23:30
 */

#ifndef SSC_ENCODE_H
#define SSC_ENCODE_H

#include <string.h>

#include "block_footer.h"
#include "block_header.h"
#include "main_header.h"
#include "main_footer.h"
#include "block_mode_marker.h"
#include "block_encode.h"
#include "kernel_chameleon_encode.h"
#include "ssc_api.h"

typedef enum {
    SSC_ENCODE_STATE_READY = 0,
    SSC_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER,
    SSC_ENCODE_STATE_STALL_ON_INPUT_BUFFER,
    SSC_ENCODE_STATE_ERROR
} SSC_ENCODE_STATE;

typedef enum {
    SSC_ENCODE_PROCESS_WRITE_BLOCKS,
    SSC_ENCODE_PROCESS_WRITE_BLOCKS_IN_TO_WORKBUFFER,
    SSC_ENCODE_PROCESS_WRITE_BLOCKS_WORKBUFFER_TO_OUT,
    SSC_ENCODE_PROCESS_WRITE_FOOTER,
    SSC_ENCODE_PROCESS_FINISHED
} SSC_ENCODE_PROCESS;

#pragma pack(push)
#pragma pack(4)

typedef struct {
    uint_fast64_t memorySize;
    uint_fast64_t outstandingBytes;
} ssc_encode_work_buffer_data;

typedef struct {
    SSC_ENCODE_PROCESS process;
    SSC_COMPRESSION_MODE compressionMode;
    SSC_BLOCK_TYPE blockType;
    const struct stat* fileAttributes;

    uint_fast64_t totalRead;
    uint_fast64_t totalWritten;

    ssc_block_encode_state blockEncodeStateA;
    ssc_block_encode_state blockEncodeStateB;

    ssc_byte_buffer* workBuffer;
    ssc_encode_work_buffer_data workBufferData;
} ssc_encode_state;
#pragma pack(pop)

SSC_ENCODE_STATE ssc_encode_init(ssc_byte_buffer *, ssc_byte_buffer *, const uint_fast64_t, ssc_encode_state *, const SSC_COMPRESSION_MODE, const SSC_ENCODE_OUTPUT_TYPE, const SSC_BLOCK_TYPE);
SSC_ENCODE_STATE ssc_encode_process(ssc_byte_buffer *, ssc_byte_buffer *, ssc_encode_state *, const ssc_bool);
SSC_ENCODE_STATE ssc_encode_finish(ssc_byte_buffer *, ssc_encode_state *);

#endif