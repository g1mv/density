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
 * 24/10/13 11:57
 *
 * -------------------
 * Chameleon algorithm
 * -------------------
 *
 * Author(s)
 * Guillaume Voirin (https://github.com/gpnuma)
 *
 * Description
 * Hash based superfast kernel
 */

#ifndef DENSITY_CHAMELEON_H
#define DENSITY_CHAMELEON_H

#include "globals.h"
#include "memory_teleport.h"
#include "density_api.h"
#include "memory_location.h"

#define DENSITY_CHAMELEON_PREFERRED_BLOCK_SIGNATURES_SHIFT                  11
#define DENSITY_CHAMELEON_PREFERRED_BLOCK_SIGNATURES                        (1 << DENSITY_CHAMELEON_PREFERRED_BLOCK_SIGNATURES_SHIFT)

#define DENSITY_CHAMELEON_PREFERRED_EFFICIENCY_CHECK_SIGNATURES_SHIFT       7
#define DENSITY_CHAMELEON_PREFERRED_EFFICIENCY_CHECK_SIGNATURES             (1 << DENSITY_CHAMELEON_PREFERRED_EFFICIENCY_CHECK_SIGNATURES_SHIFT)

#define DENSITY_CHAMELEON_HASH_BITS                                         16
#define DENSITY_CHAMELEON_HASH_MULTIPLIER                                   (uint32_t)2641295638lu

#define DENSITY_CHAMELEON_HASH_ALGORITHM(hash32, value32)                   hash32 = value32 * DENSITY_CHAMELEON_HASH_MULTIPLIER;\
                                                                            hash32 = (hash32 >> (32 - DENSITY_CHAMELEON_HASH_BITS));

typedef enum {
    DENSITY_CHAMELEON_SIGNATURE_FLAG_CHUNK = 0x0,
    DENSITY_CHAMELEON_SIGNATURE_FLAG_MAP = 0x1,
} DENSITY_CHAMELEON_SIGNATURE_FLAG;

typedef uint64_t density_chameleon_signature;

#endif