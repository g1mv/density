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
 * 14/02/18 14:30
 *
 * ---------------
 * Swift algorithm
 * ---------------
 *
 * Author(s)
 * Guillaume Voirin (https://github.com/gpnuma)
 *
 * Description
 * Hash based kernel derived from Chameleon, with very small memory footprint and quicker learning for small datasets
 */

#ifndef DENSITY_SWIFT_H
#define DENSITY_SWIFT_H

#include "../../globals.h"

#define DENSITY_SWIFT_HASH_BITS                                             8
#define DENSITY_SWIFT_HASH_MULTIPLIER                                       (uint32_t)0x9D6EF916lu

typedef enum {
    DENSITY_SWIFT_SIGNATURE_FLAG_CHUNK = 0x0,
    DENSITY_SWIFT_SIGNATURE_FLAG_MAP = 0x1,
} DENSITY_SWIFT_SIGNATURE_FLAG;

typedef uint64_t density_swift_signature;

#define DENSITY_SWIFT_MAXIMUM_COMPRESSED_BODY_SIZE_PER_SIGNATURE            (density_bitsizeof(density_swift_signature) * sizeof(uint16_t))   // Uncompressed chunks
#define DENSITY_SWIFT_DECOMPRESSED_BODY_SIZE_PER_SIGNATURE                  (density_bitsizeof(density_swift_signature) * sizeof(uint16_t))

#define DENSITY_SWIFT_MAXIMUM_COMPRESSED_UNIT_SIZE                          (sizeof(density_swift_signature) + DENSITY_SWIFT_MAXIMUM_COMPRESSED_BODY_SIZE_PER_SIGNATURE)
#define DENSITY_SWIFT_DECOMPRESSED_UNIT_SIZE                                (DENSITY_SWIFT_DECOMPRESSED_BODY_SIZE_PER_SIGNATURE)

#define DENSITY_SWIFT_WORK_BLOCK_SIZE                                       128

#endif
