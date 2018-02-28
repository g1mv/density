/*
 * Centaurean Density
 *
 * Copyright (c) 2018, Guillaume Voirin
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     1. Redistributions of source code must retain the above copyright notice, this
 *        list of conditions and the following disclaimer.
 *
 *     2. Redistributions in binary form must reproduce the above copyright notice,
 *        this list of conditions and the following disclaimer in the documentation
 *        and/or other materials provided with the distribution.
 *
 *     3. Neither the name of the copyright holder nor the names of its
 *        contributors may be used to endorse or promote products derived from
 *        this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * 14/02/18 14:37
 *
 * ---------------
 * PicoZ algorithm
 * ---------------
 *
 * Author(s)
 * Guillaume Voirin (https://github.com/gpnuma)
 *
 * Description
 * LZ based kernel derived from Chameleon, with very small memory footprint and quick learning curve
 */

#ifndef DENSITY_SWIFT_DICTIONARY_H
#define DENSITY_SWIFT_DICTIONARY_H

#include "../swift.h"

#include <string.h>

#pragma pack(push)
#pragma pack(4)
typedef struct {
    uint32_t as_uint32_t;
} density_swift_dictionary_entry;

typedef struct {
    density_swift_dictionary_entry entries[1 << DENSITY_SWIFT_HASH_BITS];
} density_swift_dictionary;

typedef struct {
    union {
        uint8_t letter_a;
        uint8_t letter_b;
    };

    uint16_t as_uint16_t;
} density_picoz_dictionary_entry_2;

typedef struct {
    uint16_t as_uint16_t;
    uint16_t as_uint8_t;
} density_picoz_dictionary_entry_3;

typedef struct {
    uint16_t as_uint16_t;
} density_picoz_dictionary_entry_4;

typedef struct {
    uint64_t as_uint64_t;
} density_picoz_dictionary_entry;

typedef struct {
    density_picoz_dictionary_entry entries[1 << 8];
} density_picoz_dictionary;
#pragma pack(pop)

#endif
