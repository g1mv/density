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
 * 19/10/13 00:02
 */

#ifndef SSC_BLOCK_ENCODE_H
#define SSC_BLOCK_ENCODE_H

#include "block_footer.h"
#include "block_header.h"
#include "main_header.h"
#include "main_footer.h"
#include "block_mode_marker.h"
#include "kernel_encode.h"

typedef enum {
    SSC_BLOCK_ENCODE_STATE_READY = 0,
    SSC_BLOCK_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER,
    SSC_BLOCK_ENCODE_STATE_STALL_ON_INPUT_BUFFER,
    SSC_BLOCK_ENCODE_STATE_ERROR
} SSC_BLOCK_ENCODE_STATE;

typedef enum {
    SSC_BLOCK_ENCODE_PROCESS_WRITE_BLOCK_HEADER,
    SSC_BLOCK_ENCODE_PROCESS_WRITE_BLOCK_MODE_MARKER,
    SSC_BLOCK_ENCODE_PROCESS_WRITE_BLOCK_FOOTER,
    SSC_BLOCK_ENCODE_PROCESS_WRITE_LAST_BLOCK_FOOTER,
    SSC_BLOCK_ENCODE_PROCESS_WRITE_DATA,
    SSC_BLOCK_ENCODE_PROCESS_FINISHED
} SSC_BLOCK_ENCODE_PROCESS;

#pragma pack(push)
#pragma pack(4)
typedef struct {
    uint_fast64_t inStart;
    uint_fast64_t outStart;
} ssc_block_encode_current_block_data;

typedef struct {
    SSC_BLOCK_ENCODE_PROCESS process;
    SSC_BLOCK_MODE targetMode;
    SSC_BLOCK_MODE currentMode;
    SSC_BLOCK_TYPE blockType;

    uint_fast64_t totalRead;
    uint_fast64_t totalWritten;

    ssc_block_encode_current_block_data currentBlockData;

    void*kernelEncodeState;
    SSC_KERNEL_ENCODE_STATE (*kernelEncodeInit)(void*);
    SSC_KERNEL_ENCODE_STATE (*kernelEncodeProcess)(ssc_byte_buffer *, ssc_byte_buffer *, void*, const ssc_bool);
    SSC_KERNEL_ENCODE_STATE (*kernelEncodeFinish)(void*);
} ssc_block_encode_state;
#pragma pack(pop)

SSC_BLOCK_ENCODE_STATE ssc_block_encode_init(ssc_block_encode_state *, const SSC_BLOCK_MODE, const SSC_BLOCK_TYPE, void*, SSC_KERNEL_ENCODE_STATE (*)(void*), SSC_KERNEL_ENCODE_STATE (*)(ssc_byte_buffer *, ssc_byte_buffer *, void*, const ssc_bool), SSC_KERNEL_ENCODE_STATE (*)(void*));
SSC_BLOCK_ENCODE_STATE ssc_block_encode_process(ssc_byte_buffer *, ssc_byte_buffer *, ssc_block_encode_state *, const ssc_bool);
SSC_BLOCK_ENCODE_STATE ssc_block_encode_finish(ssc_block_encode_state *);

#endif