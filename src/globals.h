/*
 * Centaurean Density
 *
 * Copyright (c) 2013, Guillaume Voirin
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
 * 11/10/13 02:01
 */

#ifndef DENSITY_GLOBALS_H
#define DENSITY_GLOBALS_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#include "density_api.h"

#define DENSITY_NOT_ZERO(x) (!!(x))

#define DENSITY_MINIMUM(x, y)   (((x)<(y))?(x):(y))
#define DENSITY_MAXIMUM(x, y)   (((x)>(y))?(x):(y))
#define DENSITY_MAXIMUM_3(x, y, z) (DENSITY_MAXIMUM(DENSITY_MAXIMUM(x, y), z))

#if UINTPTR_MAX == 0xffffffff
#define DENSITY_32
#elif UINTPTR_MAX == 0xffffffffffffffff
#define DENSITY_64
#else
#error
#endif

// __builtin_memcpy performance fix (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=84719)
#if defined(DENSITY_64) && !defined(__clang__) && defined(__GNUC__)
#define DENSITY_BUILTIN_MEMCPY_FASTEST_BYTE_COUNT(INTENDED) DENSITY_MAXIMUM(8, INTENDED)
#else
#define DENSITY_BUILTIN_MEMCPY_FASTEST_BYTE_COUNT(INTENDED) INTENDED
#endif

#if defined(__clang__) || defined(__GNUC__)
#define DENSITY_FORCE_INLINE		inline __attribute__((always_inline))
#define DENSITY_RESTRICT			restrict
#define DENSITY_RESTRICT_DECLARE
#define DENSITY_MEMCPY				__builtin_memcpy
#define DENSITY_MEMMOVE				__builtin_memmove
#define DENSITY_MEMSET				__builtin_memset
#define DENSITY_LIKELY(x)			__builtin_expect(DENSITY_NOT_ZERO(x), 1)
#define DENSITY_UNLIKELY(x)			__builtin_expect(DENSITY_NOT_ZERO(x), 0)
#define DENSITY_PREFETCH(x, y, z)    __builtin_prefetch(x, y, z)
#define DENSITY_CTZ(x)				__builtin_ctz(x)

#if defined(__BYTE_ORDER__)
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define DENSITY_LITTLE_ENDIAN
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define DENSITY_BIG_ENDIAN
#else
#error Unsupported endianness
#endif
#else
#error Unkwnown endianness
#endif

#elif defined(_MSC_VER)
#include <string.h>
#include <intrin.h>

#define DENSITY_FORCE_INLINE		__forceinline
#define DENSITY_RESTRICT			__restrict
#define DENSITY_RESTRICT_DECLARE	__restrict
#define DENSITY_MEMCPY				memcpy
#define DENSITY_MEMMOVE				memmove
#define DENSITY_MEMSET				memset
#define DENSITY_LIKELY(x)			(x)
#define DENSITY_UNLIKELY(x)			(x)
#define DENSITY_PREFETCH(x, y, z)	((void)(x, y, z))

DENSITY_FORCE_INLINE uint_fast8_t density_msvc_ctz(uint64_t value) {
	unsigned long trailing_zero = 0;
	if (_BitScanForward(&trailing_zero, (unsigned long)value))
		return (uint_fast8_t)trailing_zero;
	else
		return 0;
}
#define DENSITY_CTZ(x)				density_msvc_ctz(x)

#define DENSITY_LITTLE_ENDIAN	// Little endian by default on Windows

#else
#error Unsupported compiler
#endif

#ifdef DENSITY_LITTLE_ENDIAN
#define DENSITY_CORRECT_ENDIANNESS_64(b)   ((uint64_t)(b))
#define DENSITY_CORRECT_ENDIANNESS_32(b)   ((uint32_t)(b))
#define DENSITY_CORRECT_ENDIANNESS_16(b)   ((uint16_t)(b))
#define DENSITY_CORRECT_ENDIANNESS_8(b)    (b)  // Used by generator macros
#elif defined(DENSITY_BIG_ENDIAN)
#if __GNUC__ * 100 + __GNUC_MINOR__ >= 403
#define DENSITY_CORRECT_ENDIANNESS_64(b)   __builtin_bswap64(b)
#define DENSITY_CORRECT_ENDIANNESS_32(b)   __builtin_bswap32(b)
#define DENSITY_CORRECT_ENDIANNESS_16(b)   __builtin_bswap16(b)
#define DENSITY_CORRECT_ENDIANNESS_8(b)    (b)
#else
#warning Using bulk byte swap routines. Expect performance issues.
#define DENSITY_CORRECT_ENDIANNESS_64(b)   ((((b) & 0xFF00000000000000ull) >> 56) | (((b) & 0x00FF000000000000ull) >> 40) | (((b) & 0x0000FF0000000000ull) >> 24) | (((b) & 0x000000FF00000000ull) >> 8) | (((b) & 0x00000000FF000000ull) << 8) | (((b) & 0x0000000000FF0000ull) << 24ull) | (((b) & 0x000000000000FF00ull) << 40) | (((b) & 0x00000000000000FFull) << 56))
#define DENSITY_CORRECT_ENDIANNESS_32(b)   ((((b) & 0xFF000000) >> 24) | (((b) & 0x00FF0000) >> 8) | (((b) & 0x0000FF00) << 8) | (((b) & 0x000000FF) << 24))
#define DENSITY_CORRECT_ENDIANNESS_16(b)   ((((b) & 0xFF00) >> 8) | (((b) & 0x00FF) << 8))
#define DENSITY_CORRECT_ENDIANNESS_8(b)    (b)
#endif
#else
#error Unsupported endianness
#endif

#define DENSITY_FORMAT(v)               0##v##llu

#define DENSITY_ISOLATE(b, p)           ((DENSITY_FORMAT(b) / (p)) & 0x1)

#define DENSITY_BINARY_TO_UINT(b)        ((DENSITY_ISOLATE(b, 1llu) ? 0x1 : 0)\
                                        + (DENSITY_ISOLATE(b, 8llu) ? 0x2 : 0)\
                                        + (DENSITY_ISOLATE(b, 64llu) ? 0x4 : 0)\
                                        + (DENSITY_ISOLATE(b, 512llu) ? 0x8 : 0)\
                                        + (DENSITY_ISOLATE(b, 4096llu) ? 0x10 : 0)\
                                        + (DENSITY_ISOLATE(b, 32768llu) ? 0x20 : 0)\
                                        + (DENSITY_ISOLATE(b, 262144llu) ? 0x40 : 0)\
                                        + (DENSITY_ISOLATE(b, 2097152llu) ? 0x80 : 0)\
                                        + (DENSITY_ISOLATE(b, 16777216llu) ? 0x100 : 0)\
                                        + (DENSITY_ISOLATE(b, 134217728llu) ? 0x200 : 0)\
                                        + (DENSITY_ISOLATE(b, 1073741824llu) ? 0x400 : 0)\
                                        + (DENSITY_ISOLATE(b, 8589934592llu) ? 0x800 : 0)\
                                        + (DENSITY_ISOLATE(b, 68719476736llu) ? 0x1000 : 0)\
                                        + (DENSITY_ISOLATE(b, 549755813888llu) ? 0x2000 : 0)\
                                        + (DENSITY_ISOLATE(b, 4398046511104llu) ? 0x4000 : 0)\
                                        + (DENSITY_ISOLATE(b, 35184372088832llu) ? 0x8000 : 0)\
                                        + (DENSITY_ISOLATE(b, 281474976710656llu) ? 0x10000 : 0)\
                                        + (DENSITY_ISOLATE(b, 2251799813685248llu) ? 0x20000 : 0))

#define DENSITY_UNROLL_2(op)     op; op;
#define DENSITY_UNROLL_3(op)     op; op; op;
#define DENSITY_UNROLL_4(op)     DENSITY_UNROLL_2(op)    DENSITY_UNROLL_2(op)
#define DENSITY_UNROLL_5(op)     DENSITY_UNROLL_2(op)    DENSITY_UNROLL_3(op)
#define DENSITY_UNROLL_7(op)     DENSITY_UNROLL_2(op)    DENSITY_UNROLL_5(op)
#define DENSITY_UNROLL_8(op)     DENSITY_UNROLL_4(op)    DENSITY_UNROLL_4(op)
#define DENSITY_UNROLL_9(op)     DENSITY_UNROLL_4(op)    DENSITY_UNROLL_5(op)
#define DENSITY_UNROLL_15(op)    DENSITY_UNROLL_7(op)    DENSITY_UNROLL_8(op)
#define DENSITY_UNROLL_16(op)    DENSITY_UNROLL_8(op)    DENSITY_UNROLL_8(op)
#define DENSITY_UNROLL_31(op)    DENSITY_UNROLL_16(op)    DENSITY_UNROLL_15(op)
#define DENSITY_UNROLL_32(op)    DENSITY_UNROLL_16(op)    DENSITY_UNROLL_16(op)
#define DENSITY_UNROLL_64(op)    DENSITY_UNROLL_32(op)    DENSITY_UNROLL_32(op)

#define DENSITY_CASE_GENERATOR_4(op_a, flag_a, op_b, flag_b, op_c, flag_c, op_d, flag_d, op_mid, shift)\
    case (((flag_d) << ((shift) * 3)) | ((flag_c) << ((shift) * 2)) | ((flag_b) << (shift)) | (flag_a)):\
        op_a;\
        op_mid;\
        op_b;\
        op_mid;\
        op_c;\
        op_mid;\
        op_d;\
        break;

#define DENSITY_CASE_GENERATOR_4_4_LAST_1_COMBINED(op_1, flag_1, op_2, flag_2, op_3, flag_3, op_a, flag_a, op_b, flag_b, op_c, flag_c, op_d, flag_d, op_mid, shift)\
    DENSITY_CASE_GENERATOR_4(op_1, flag_1, op_2, flag_2, op_3, flag_3, op_a, flag_a, op_mid, shift);\
    DENSITY_CASE_GENERATOR_4(op_1, flag_1, op_2, flag_2, op_3, flag_3, op_b, flag_b, op_mid, shift);\
    DENSITY_CASE_GENERATOR_4(op_1, flag_1, op_2, flag_2, op_3, flag_3, op_c, flag_c, op_mid, shift);\
    DENSITY_CASE_GENERATOR_4(op_1, flag_1, op_2, flag_2, op_3, flag_3, op_d, flag_d, op_mid, shift);

#define DENSITY_CASE_GENERATOR_4_4_LAST_2_COMBINED(op_1, flag_1, op_2, flag_2, op_a, flag_a, op_b, flag_b, op_c, flag_c, op_d, flag_d, op_mid, shift)\
    DENSITY_CASE_GENERATOR_4_4_LAST_1_COMBINED(op_1, flag_1, op_2, flag_2, op_a, flag_a, op_a, flag_a, op_b, flag_b, op_c, flag_c, op_d, flag_d, op_mid, shift);\
    DENSITY_CASE_GENERATOR_4_4_LAST_1_COMBINED(op_1, flag_1, op_2, flag_2, op_b, flag_b, op_a, flag_a, op_b, flag_b, op_c, flag_c, op_d, flag_d, op_mid, shift);\
    DENSITY_CASE_GENERATOR_4_4_LAST_1_COMBINED(op_1, flag_1, op_2, flag_2, op_c, flag_c, op_a, flag_a, op_b, flag_b, op_c, flag_c, op_d, flag_d, op_mid, shift);\
    DENSITY_CASE_GENERATOR_4_4_LAST_1_COMBINED(op_1, flag_1, op_2, flag_2, op_d, flag_d, op_a, flag_a, op_b, flag_b, op_c, flag_c, op_d, flag_d, op_mid, shift);

#define DENSITY_CASE_GENERATOR_4_4_LAST_3_COMBINED(op_1, flag_1, op_a, flag_a, op_b, flag_b, op_c, flag_c, op_d, flag_d, op_mid, shift)\
    DENSITY_CASE_GENERATOR_4_4_LAST_2_COMBINED(op_1, flag_1, op_a, flag_a, op_a, flag_a, op_b, flag_b, op_c, flag_c, op_d, flag_d, op_mid, shift);\
    DENSITY_CASE_GENERATOR_4_4_LAST_2_COMBINED(op_1, flag_1, op_b, flag_b, op_a, flag_a, op_b, flag_b, op_c, flag_c, op_d, flag_d, op_mid, shift);\
    DENSITY_CASE_GENERATOR_4_4_LAST_2_COMBINED(op_1, flag_1, op_c, flag_c, op_a, flag_a, op_b, flag_b, op_c, flag_c, op_d, flag_d, op_mid, shift);\
    DENSITY_CASE_GENERATOR_4_4_LAST_2_COMBINED(op_1, flag_1, op_d, flag_d, op_a, flag_a, op_b, flag_b, op_c, flag_c, op_d, flag_d, op_mid, shift);

#define DENSITY_CASE_GENERATOR_4_4_COMBINED(op_a, flag_a, op_b, flag_b, op_c, flag_c, op_d, flag_d, op_mid, shift)\
    DENSITY_CASE_GENERATOR_4_4_LAST_3_COMBINED(op_a, flag_a, op_a, flag_a, op_b, flag_b, op_c, flag_c, op_d, flag_d, op_mid, shift);\
    DENSITY_CASE_GENERATOR_4_4_LAST_3_COMBINED(op_b, flag_b, op_a, flag_a, op_b, flag_b, op_c, flag_c, op_d, flag_d, op_mid, shift);\
    DENSITY_CASE_GENERATOR_4_4_LAST_3_COMBINED(op_c, flag_c, op_a, flag_a, op_b, flag_b, op_c, flag_c, op_d, flag_d, op_mid, shift);\
    DENSITY_CASE_GENERATOR_4_4_LAST_3_COMBINED(op_d, flag_d, op_a, flag_a, op_b, flag_b, op_c, flag_c, op_d, flag_d, op_mid, shift);


#define DENSITY_ADD_2_2 4
#define DENSITY_ADD_2_4 4
#define DENSITY_ADD_4_2 6
#define DENSITY_ADD_4_4 8
#define DENSITY_ADD_6_2 8
#define DENSITY_ADD_6_4 10
#define DENSITY_ADD_8_2 10
#define DENSITY_ADD_8_4 12
#define DENSITY_ADD_8_8 16
#define DENSITY_ADD_16_8 24

#define DENSITY_ADD(x, y) DENSITY_ADD_##x##_##y

#define DENSITY_PASTE_CONCAT(x, y) x##y
#define DENSITY_EVAL_CONCAT(x, y) DENSITY_PASTE_CONCAT(x,y)

#ifdef DENSITY_LITTLE_ENDIAN
#define DENSITY_ENDIAN_COPY(TARGET, SOURCE, BITS)   DENSITY_MEMCPY(&(TARGET), &(SOURCE), DENSITY_BUILTIN_MEMCPY_FASTEST_BYTE_COUNT((BITS) >> 3))
#elif defined(DENSITY_BIG_ENDIAN)
#define DENSITY_ENDIAN_COPY(TARGET, SOURCE, BITS)\
const DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(uint, BITS), _t) DENSITY_EVAL_CONCAT(endian_, BITS) = DENSITY_EVAL_CONCAT(DENSITY_CORRECT_ENDIANNESS_, BITS)(SOURCE);\
DENSITY_MEMCPY(&(TARGET), &DENSITY_EVAL_CONCAT(endian_, BITS), DENSITY_BUILTIN_MEMCPY_FASTEST_BYTE_COUNT((BITS) >> 3));
#else
#error
#endif

#define density_bitsizeof(x) (8 * sizeof(x))

/**********************************************************************************************************************
 *                                                                                                                    *
 * Global compile-time switches                                                                                       *
 *                                                                                                                    *
 **********************************************************************************************************************/

#define DENSITY_MAJOR_VERSION   0
#define DENSITY_MINOR_VERSION   15
#define DENSITY_REVISION        0

#endif
