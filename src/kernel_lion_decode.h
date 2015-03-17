/*
 * Centaurean Density
 *
 * Copyright (c) 2015, Guillaume Voirin
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
 * 8/03/15 11:59
 *
 * --------------
 * Lion algorithm
 * --------------
 *
 * Author(s)
 * Guillaume Voirin (https://github.com/gpnuma)
 *
 * Description
 * Multiform compression algorithm
 */

#ifndef DENSITY_LION_DECODE_H
#define DENSITY_LION_DECODE_H

#include "kernel_lion_dictionary.h"
#include "kernel_lion.h"
#include "kernel_lion_form_model.h"
#include "block.h"
#include "kernel_decode.h"
#include "density_api.h"
#include "memory_teleport.h"
#include "block_footer.h"
#include "main_footer.h"
#include "block_mode_marker.h"
#include "kernel_lion_encode.h"
#include "main_header.h"

#define DENSITY_LION_DECODE_ITERATIONS_SHIFT                                    1

#define DENSITY_LION_DECODE_CHUNKS_PER_PROCESS_UNIT              4
#define DENSITY_LION_DECODE_PROCESS_UNIT_SIZE                   (DENSITY_LION_ENCODE_CHUNKS_PER_PROCESS_UNIT * sizeof(uint32_t) * 8)

typedef enum {
    DENSITY_LION_DECODE_PROCESS_CHECK_BLOCK_STATE,
    DENSITY_LION_DECODE_PROCESS_CHECK_OUTPUT_SIZE,
    DENSITY_LION_DECODE_PROCESS_UNIT,
} DENSITY_LION_DECODE_PROCESS;

#define DENSITY_LION_DECODE_BITMASK_VALUES {\
    BINARY_TO_UINT(0),\
    BINARY_TO_UINT(1),\
    BINARY_TO_UINT(11),\
    BINARY_TO_UINT(111),\
    BINARY_TO_UINT(1111),\
    BINARY_TO_UINT(11111),\
    BINARY_TO_UINT(111111),\
    BINARY_TO_UINT(1111111),\
}

#define DENSITY_LION_DECODE_NUMBER_OF_BITMASK_VALUES                                    8

#pragma pack(push)
#pragma pack(4)
typedef struct {
    DENSITY_LION_DECODE_PROCESS process;

    density_main_header_parameters parameters;
    uint_fast64_t resetCycle;

    uint_fast32_t shift;
    density_lion_signature signature;
    uint_fast64_t chunksCount;
    bool efficiencyChecked;
    bool readSignature;

    uint_fast64_t endDataOverhead;

    density_lion_form_data formData;
    density_lion_unigram_data unigramData;

    uint_fast32_t lastHash;
    uint_fast8_t lastUnigram;

    density_lion_dictionary dictionary;
} density_lion_decode_state;
#pragma pack(pop)

static const uint8_t density_lion_decode_bitmasks[DENSITY_LION_DECODE_NUMBER_OF_BITMASK_VALUES] = DENSITY_LION_DECODE_BITMASK_VALUES;

DENSITY_KERNEL_DECODE_STATE density_lion_decode_init(density_lion_decode_state *, const density_main_header_parameters parameters, const uint_fast32_t);
DENSITY_KERNEL_DECODE_STATE density_lion_decode_continue(density_memory_teleport *, density_memory_location *, density_lion_decode_state *);
DENSITY_KERNEL_DECODE_STATE density_lion_decode_finish(density_memory_teleport *, density_memory_location *, density_lion_decode_state *);

#endif