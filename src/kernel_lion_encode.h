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
 * 06/12/13 20:20
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

#ifndef DENSITY_LION_ENCODE_H
#define DENSITY_LION_ENCODE_H

#include "kernel_lion_dictionary.h"
#include "kernel_lion.h"
#include "block.h"
#include "kernel_encode.h"
#include "density_api.h"
#include "memory_location.h"
#include "memory_teleport.h"

#define DENSITY_LION_ENCODE_MINIMUM_OUTPUT_LOOKAHEAD             (2 * (sizeof(density_lion_signature) + sizeof(uint32_t) * 4 * sizeof(density_lion_signature)))
#define DENSITY_LION_ENCODE_PROCESS_UNIT_SIZE                    (2 * 4 * sizeof(uint64_t))

typedef enum {
    DENSITY_LION_ENCODE_PROCESS_PREPARE_NEW_BLOCK,
    DENSITY_LION_ENCODE_PROCESS_CHECK_SIGNATURE_STATE,
    DENSITY_LION_ENCODE_PROCESS_READ_CHUNK,
} DENSITY_LION_ENCODE_PROCESS;

typedef enum {
    DENSITY_LION_FORM_PREDICTIONS,
    DENSITY_LION_FORM_DICTIONARY_A,
    DENSITY_LION_FORM_DICTIONARY_B,
    DENSITY_LION_FORM_SECONDARY,
} DENSITY_LION_FORM;

typedef struct {
    uint32_t usage;
    uint8_t rank;
} density_lion_form_statistics;

typedef struct {
    density_lion_form_statistics *statistics;
} density_lion_form_rank;

#pragma pack(push)
#pragma pack(4)
typedef struct {
    DENSITY_LION_ENCODE_PROCESS process;

#if DENSITY_ENABLE_PARALLELIZABLE_DECOMPRESSIBLE_OUTPUT == DENSITY_YES
    uint_fast64_t resetCycle;
#endif

    uint_fast32_t shift;
    density_lion_signature * signature;
    uint_fast32_t signaturesCount;
    uint_fast8_t efficiencyChecked;

    density_lion_form_statistics formStatistics[4];
    density_lion_form_rank formRanks[4];

    uint_fast32_t lastHash;
    uint_fast32_t lastChunk;

    uint_fast64_t localSignature;
    uint_fast8_t localShift;

    density_lion_dictionary dictionary;
} density_lion_encode_state;
#pragma pack(pop)

void density_lion_encode_push_to_signature(density_memory_location *, density_lion_encode_state *, uint64_t, uint_fast8_t);

DENSITY_KERNEL_ENCODE_STATE density_lion_encode_init(density_lion_encode_state *);
DENSITY_KERNEL_ENCODE_STATE density_lion_encode_continue(density_memory_teleport *, density_memory_location *, density_lion_encode_state *, const density_bool);
DENSITY_KERNEL_ENCODE_STATE density_lion_encode_finish(density_memory_teleport *, density_memory_location *, density_lion_encode_state *);
#endif