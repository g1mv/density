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
 * 24/10/13 12:01
 *
 * -------------------
 * Chameleon algorithm
 * -------------------
 *
 * Author(s)
 * Guillaume Voirin
 *
 * Description
 * Hash based superfast kernel
 */

#ifndef DENSITY_CHAMELEON_ENCODE_H
#define DENSITY_CHAMELEON_ENCODE_H

#include "byte_buffer.h"
#include "kernel_chameleon_dictionary.h"
#include "kernel_chameleon.h"
#include "block.h"
#include "kernel_encode.h"
#include "density_api.h"

#define DENSITY_CHAMELEON_ENCODE_MINIMUM_OUTPUT_LOOKAHEAD             (sizeof(density_chameleon_signature) + sizeof(uint32_t) * 8 * sizeof(density_chameleon_signature))

typedef enum {
    DENSITY_CHAMELEON_ENCODE_PROCESS_CHECK_STATE,
    DENSITY_CHAMELEON_ENCODE_PROCESS_PREPARE_NEW_BLOCK,
    DENSITY_CHAMELEON_ENCODE_PROCESS_DATA,
    DENSITY_CHAMELEON_ENCODE_PROCESS_FINISH
} DENSITY_CHAMELEON_ENCODE_PROCESS;

#pragma pack(push)
#pragma pack(4)
typedef struct {
    DENSITY_CHAMELEON_ENCODE_PROCESS process;

#if DENSITY_ENABLE_PARALLELIZATION == DENSITY_YES
    uint_fast64_t resetCycle;
#endif

    uint_fast32_t shift;
    density_chameleon_signature * signature;
    uint_fast32_t signaturesCount;
    uint_fast8_t efficiencyChecked;

    density_chameleon_dictionary dictionary;
} density_chameleon_encode_state;
#pragma pack(pop)

DENSITY_KERNEL_ENCODE_STATE density_chameleon_encode_init(density_chameleon_encode_state *);
DENSITY_KERNEL_ENCODE_STATE density_chameleon_encode_process(density_byte_buffer *, density_byte_buffer *, density_chameleon_encode_state *, const density_bool);
DENSITY_KERNEL_ENCODE_STATE density_chameleon_encode_finish(density_chameleon_encode_state *);

#endif