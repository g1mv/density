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
 * 5/03/15 17:48
 */

#ifndef DENSITY_KERNEL_H
#define DENSITY_KERNEL_H

#include "globals.h"
#include "memory_location.h"

typedef uint64_t density_kernel_signature;

typedef struct {
    uint_fast8_t shift;
    density_kernel_signature registerSignature;
    density_kernel_signature *memorySignature;
    uint_fast32_t count;
} density_kernel_signature_data;

#define DENSITY_KERNEL_ENCODE_PREPARE_NEW_SIGNATURE(out, data) {\
    (data)->count++;\
    (data)->shift = 0;\
    (data)->registerSignature = 0;\
    (data)->memorySignature = (density_kernel_signature *) ((out)->pointer);\
    (out)->pointer += sizeof(density_kernel_signature);\
}

#define DENSITY_KERNEL_ENCODE_PUSH_REGISTER_SIGNATURE_TO_MEMORY(data) {\
    *((data)->memorySignature) = (data)->registerSignature;\
}

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define DENSITY_KERNEL_ENCODE_PUSH_TO_SIGNATURE_NO_CHECK(signature, shift, content, bits) {\
    (signature) |= ((content) << (shift));\
    (shift) += (bits);\
}
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define density_kernel_encode_push_to_signature_no_check(signatureData, content, bits) {\
    (signature) |= ((content) << ((56 - ((shift) & ~0x7)) + ((shift) & 0x7)));\
    (shift) += (bits);\
}
#elif
#error Unknow endianness
#endif

#define DENSITY_KERNEL_ENCODE_PUSH_TO_SIGNATURE(out, data, content, bits) {\
    DENSITY_KERNEL_ENCODE_PUSH_TO_SIGNATURE_NO_CHECK((data)->registerSignature, (data)->shift, content, bits);\
\
    if (density_unlikely((data)->shift & 0xFFFFFFFFFFFFFFC0llu)) {\
        DENSITY_KERNEL_ENCODE_PUSH_REGISTER_SIGNATURE_TO_MEMORY((data));\
\
        const uint8_t remainder = (uint_fast8_t) ((data)->shift & 0x3F);\
        DENSITY_KERNEL_ENCODE_PREPARE_NEW_SIGNATURE((out), (data));\
        if (remainder)\
            DENSITY_KERNEL_ENCODE_PUSH_TO_SIGNATURE_NO_CHECK((data)->registerSignature, (data)->shift, (content) >> ((bits) - remainder), remainder);\
    }\
}

#endif