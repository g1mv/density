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
 * 9/03/15 14:45
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

#ifndef DENSITY_LION_UNIGRAM_MODEL_H
#define DENSITY_LION_UNIGRAM_MODEL_H

#include <stdbool.h>
#include "globals.h"
#include "kernel_lion.h"

#define DENSITY_LION_NUMBER_OF_UNIGRAMS                                    (1 << density_bitsizeof(uint8_t))
#define DENSITY_LION_UNIGRAM_MODEL_UPDATE_FREQUENCY                         0xF

#pragma pack(push)
#pragma pack(4)
typedef struct {
    uint8_t unigram;
    uint8_t rank;
    bool qualified;
    void *previousUnigram;
} density_lion_unigram_node;

typedef struct {
    uint8_t nextAvailableUnigram;
    density_lion_unigram_node unigramsPool[DENSITY_LION_NUMBER_OF_UNIGRAMS];
    density_lion_unigram_node *lastUnigramNode;
    density_lion_unigram_node *unigramsIndex[DENSITY_LION_NUMBER_OF_UNIGRAMS];
} density_lion_unigram_data;
#pragma pack(pop)

void density_lion_unigram_model_init(density_lion_unigram_data *);
void density_lion_unigram_model_update(density_lion_unigram_data *, const uint8_t, density_lion_unigram_node *);

#endif
