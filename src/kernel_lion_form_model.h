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
 * 9/03/15 12:04
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

#ifndef DENSITY_LION_FORM_MODEL_H
#define DENSITY_LION_FORM_MODEL_H

#include "globals.h"
#include "kernel_lion.h"

#define DENSITY_LION_NUMBER_OF_FORMS                                    4

// Unary codes except the last one
#define DENSITY_LION_FORM_MODEL_ENTROPY_CODES {\
    {BINARY_TO_UINT(0), 1},\
    {BINARY_TO_UINT(10), 2},\
    {BINARY_TO_UINT(110), 3},\
    {BINARY_TO_UINT(111), 3},\
}

#pragma pack(push)
#pragma pack(4)
typedef struct {
    DENSITY_LION_FORM form;
    uint32_t usage;
    uint8_t rank;
    void *previousForm;
} density_lion_form_node;

typedef struct {
    uint8_t nextAvailableForm;
    density_lion_form_node formsPool[DENSITY_LION_NUMBER_OF_FORMS];
    density_lion_form_node *formsIndex[DENSITY_LION_NUMBER_OF_FORMS];
} density_lion_form_data;
#pragma pack(pop)

static const density_lion_entropy_code density_lion_form_entropy_codes[DENSITY_LION_NUMBER_OF_FORMS] = DENSITY_LION_FORM_MODEL_ENTROPY_CODES;

void density_lion_form_model_init(density_lion_form_data *);
void density_lion_form_model_update(density_lion_form_data *, density_lion_form_node *, density_lion_form_node *);
density_lion_entropy_code density_lion_form_model_get_encoding(density_lion_form_data *, const DENSITY_LION_FORM);

#endif