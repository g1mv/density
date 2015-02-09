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
 * 5/02/15 20:54
 *
 * ------------------
 * Argonaut algorithm
 * ------------------
 *
 * Author(s)
 * Guillaume Voirin (https://github.com/gpnuma)
 *
 * Description
 * Word rank and predictions algorithm
 */

#ifndef DENSITY_ARGONAUT_ENCODE_H
#define DENSITY_ARGONAUT_ENCODE_H

#include "block.h"
#include "kernel_encode.h"
#include "kernel_argonaut_dictionary.h"
#include "kernel_argonaut.h"
#include "main_encode.h"
#include "memory_location.h"

#include <inttypes.h>
#include <math.h>
#include <stdint.h>

#define density_argonaut_contains_zero(search64) (((search64) - 0x0101010101010101llu) & ~(search64) & 0x8080808080808080llu)
#define density_argonaut_contains_value(search64, value8) (density_argonaut_contains_zero((search64) ^ (~0llu / 255 * (value8))))

#define DENSITY_ARGONAUT_BLOCK   512

typedef enum {
    DENSITY_ARGONAUT_ENCODE_PROCESS_CHECK_OUTPUT_MEMORY,
    DENSITY_ARGONAUT_ENCODE_PROCESS_GOTO_NEXT_WORD,
    DENSITY_ARGONAUT_ENCODE_PROCESS_WORD,
    DENSITY_ARGONAUT_ENCODE_PROCESS_FINISH
} DENSITY_ARGONAUT_ENCODE_PROCESS;

#pragma pack(push)
#pragma pack(1)
typedef struct {
    DENSITY_ARGONAUT_ENCODE_PROCESS process;

    uint_fast32_t shift;
    uint_fast16_t count;

    density_argonaut_encode_word word;

    uint_fast64_t buffer;
    uint_fast8_t bufferBits;

    density_argonaut_dictionary dictionary;
} density_argonaut_encode_state;
#pragma pack(pop)

DENSITY_KERNEL_ENCODE_STATE density_argonaut_encode_init(density_argonaut_encode_state *);

DENSITY_KERNEL_ENCODE_STATE density_argonaut_encode_continue(density_memory_teleport *, density_memory_location *, density_argonaut_encode_state *);

DENSITY_KERNEL_ENCODE_STATE density_argonaut_encode_finish(density_memory_teleport *, density_memory_location *, density_argonaut_encode_state *);

#endif