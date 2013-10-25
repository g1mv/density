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
 * 19/10/13 00:05
 */

#ifndef SSC_BLOCK_DECODE_H
#define SSC_BLOCK_DECODE_H

#include "block_header.h"
#include "block_footer.h"
#include "byte_buffer.h"
#include "main_header.h"
#include "main_footer.h"
#include "block_mode_marker.h"
#include "kernel_decode.h"

typedef enum {
    SSC_BLOCK_DECODE_STATE_READY = 0,
    SSC_BLOCK_DECODE_STATE_STALL_ON_OUTPUT_BUFFER,
    SSC_BLOCK_DECODE_STATE_STALL_ON_INPUT_BUFFER,
    SSC_BLOCK_DECODE_STATE_ERROR
} SSC_BLOCK_DECODE_STATE;

typedef enum {
    SSC_BLOCK_DECODE_PROCESS_READ_BLOCK_HEADER,
    SSC_BLOCK_DECODE_PROCESS_READ_BLOCK_MODE_MARKER,
    SSC_BLOCK_DECODE_PROCESS_READ_BLOCK_FOOTER,
    SSC_BLOCK_DECODE_PROCESS_READ_LAST_BLOCK_FOOTER,
    SSC_BLOCK_DECODE_PROCESS_READ_DATA,
    SSC_BLOCK_DECODE_PROCESS_FINISHED
} SSC_BLOCK_DECODE_PROCESS;

#pragma pack(push)
#pragma pack(4)
typedef struct {
    uint_fast64_t inStart;
    uint_fast64_t outStart;
} ssc_block_decode_current_block_data;

typedef struct {
    SSC_BLOCK_DECODE_PROCESS process;
    SSC_BLOCK_MODE mode;
    SSC_BLOCK_TYPE blockType;

    uint_fast64_t totalRead;
    uint_fast64_t totalWritten;
    uint_fast32_t endDataOverhead;

    ssc_block_header lastBlockHeader;
    ssc_mode_marker lastModeMarker;
    ssc_block_footer lastBlockFooter;

    ssc_block_decode_current_block_data currentBlockData;

    void* kernelDecodeState;
    SSC_KERNEL_DECODE_STATE (*kernelDecodeInit)(void*, const uint32_t);
    SSC_KERNEL_DECODE_STATE (*kernelDecodeProcess)(ssc_byte_buffer *, ssc_byte_buffer *, void*, const ssc_bool);
    SSC_KERNEL_DECODE_STATE (*kernelDecodeFinish)(void*);
} ssc_block_decode_state;
#pragma pack(pop)

SSC_BLOCK_DECODE_STATE ssc_block_decode_init(ssc_block_decode_state *, const SSC_BLOCK_MODE, const SSC_BLOCK_TYPE, const uint_fast32_t, void*, SSC_KERNEL_DECODE_STATE (*)(void*, const uint32_t), SSC_KERNEL_DECODE_STATE (*)(ssc_byte_buffer *, ssc_byte_buffer *, void*, const ssc_bool), SSC_KERNEL_DECODE_STATE (*)(void*));
SSC_BLOCK_DECODE_STATE ssc_block_decode_process(ssc_byte_buffer *, ssc_byte_buffer *, ssc_block_decode_state *, const ssc_bool);
SSC_BLOCK_DECODE_STATE ssc_block_decode_finish(ssc_block_decode_state *);

#endif