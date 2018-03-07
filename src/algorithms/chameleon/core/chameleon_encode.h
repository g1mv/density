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
 * 23/06/15 21:51
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

#ifndef DENSITY_CHAMELEON_ENCODE_H
#define DENSITY_CHAMELEON_ENCODE_H

#include "../dictionary/chameleon_dictionary.h"
#include "../../algorithms.h"

#define DENSITY_CHAMELEON_ENCODE_PREPARE_SIGNATURE \
    signature = 0;\
    signature_pointer = (density_chameleon_signature *) *out;\
    *out += sizeof(density_chameleon_signature);

#define DENSITY_CHAMELEON_ENCODE_PUSH_SIGNATURE \
    DENSITY_MEMCPY(signature_pointer, &signature, sizeof(density_chameleon_signature));\
    DENSITY_CHAMELEON_ENCODE_PREPARE_SIGNATURE;

#define DENSITY_CHAMELEON_ENCODE_CLEAR_DICTIONARY(HASH_BITS, SPAN) \
    const uint_fast32_t step = ((((uint32_t)1 << (HASH_BITS)) - 256) / (SPAN)) + 1;\
    const uint_fast32_t start = ((uint32_t)1 << 8) + transition_counter * step;\
    const uint_fast32_t end = DENSITY_MINIMUM(start + step, (uint32_t)1 << (HASH_BITS));\
    DENSITY_ALGORITHMS_PRINT_CLEAR(start, end);\
    for(uint_fast32_t counter = start; counter < end; counter ++) {\
        const uint64_t bitmap = dictionary->bitmap[counter >> 6];\
        const uint64_t mask = ((uint64_t) 1 << (counter & 0x3f));\
        dictionary->entries[counter] = DENSITY_NOT_ZERO(bitmap & mask) * dictionary->entries[counter];\
    }

#define DENSITY_CHAMELEON_ENCODE_GENERATE_FAST_COMPRESSION_UNIT(COPY_BYTES, HASH_BITS, BYTE_GROUP_SIZE) \
    DENSITY_MEMCPY(&memcopy_64, &(*in)[in_position], DENSITY_BUILTIN_MEMCPY_FASTEST_BYTE_COUNT(COPY_BYTES));\
    unit = DENSITY_ALGORITHMS_EXTRACT_64(memcopy_64, BYTE_GROUP_SIZE);\
    hash = DENSITY_ALGORITHMS_MULTIPLY_SHIFT_64(unit, HASH_BITS);\
    value = &dictionary->entries[hash];\
    in_position += (BYTE_GROUP_SIZE);\
    switch (unit ^ *value) {\
        case 0:\
            signature |= ((uint64_t) DENSITY_CHAMELEON_SIGNATURE_FLAG_MAP << shift);\
            DENSITY_MEMCPY(*out, &hash, DENSITY_BUILTIN_MEMCPY_FASTEST_BYTE_COUNT((HASH_BITS) >> 3));\
            *out += ((HASH_BITS) >> 3);\
            break;\
        default:\
            *value = unit;\
            DENSITY_MEMCPY(*out, &unit, DENSITY_BUILTIN_MEMCPY_FASTEST_BYTE_COUNT(BYTE_GROUP_SIZE));\
            *out += (BYTE_GROUP_SIZE);\
            break;\
    }

#define DENSITY_CHAMELEON_ENCODE_GENERATE_STUDY_COMPRESSION_UNIT(HASH_BITS, BYTE_GROUP_SIZE) \
    DENSITY_MEMCPY(&memcopy_64, &(*in)[in_position], DENSITY_BUILTIN_MEMCPY_FASTEST_BYTE_COUNT(BYTE_GROUP_SIZE));\
    unit = DENSITY_ALGORITHMS_EXTRACT_64(memcopy_64, BYTE_GROUP_SIZE);\
    hash = DENSITY_ALGORITHMS_MULTIPLY_SHIFT_64(unit, HASH_BITS);\
    value = &dictionary->entries[hash];\
    in_position += (BYTE_GROUP_SIZE);\
    switch (unit ^ *value) {\
        case 0:\
            hits++;\
            signature |= ((uint64_t) DENSITY_CHAMELEON_SIGNATURE_FLAG_MAP << shift);\
            DENSITY_MEMCPY(*out, &hash, DENSITY_BUILTIN_MEMCPY_FASTEST_BYTE_COUNT((HASH_BITS) >> 3));\
            *out += ((HASH_BITS) >> 3);\
            break;\
        default:\
            *value = unit;\
            uint64_t *const bitmap = &dictionary->bitmap[hash >> 6];\
            const uint64_t mask = ((uint64_t) 1 << (hash & 0x3f));\
            const bool was_not_set = !(*bitmap & mask);\
            inserts += was_not_set;\
            collisions += !was_not_set;\
            *bitmap = *bitmap | mask;\
            DENSITY_MEMCPY(*out, &unit, DENSITY_BUILTIN_MEMCPY_FASTEST_BYTE_COUNT(BYTE_GROUP_SIZE));\
            *out += (BYTE_GROUP_SIZE);\
            break;\
    }

#define DENSITY_CHAMELEON_ENCODE_GENERATE_TRANSITION_TO(HASH_BITS, BYTE_GROUP_SIZE, NEXT_HASH_BITS, NEXT_GROUP_BYTE_SIZE, SPAN, SPECIAL_INSTRUCTIONS) \
    DENSITY_ALGORITHMS_PRINT_TRANSITION(HASH_BITS, BYTE_GROUP_SIZE, NEXT_HASH_BITS, NEXT_GROUP_BYTE_SIZE, SPAN)\
    total_inserts = 0;\
    transition_counter = SPAN;\
    while (DENSITY_LIKELY(in_position < in_limit && transition_counter)) {\
        for(uint_fast8_t shift = 0; shift < 0x40; shift++) {\
            DENSITY_CHAMELEON_ENCODE_GENERATE_FAST_COMPRESSION_UNIT(DENSITY_MAXIMUM(BYTE_GROUP_SIZE, NEXT_GROUP_BYTE_SIZE), HASH_BITS, BYTE_GROUP_SIZE);\
            const uint64_t new_unit = DENSITY_ALGORITHMS_EXTRACT_64(memcopy_64, NEXT_GROUP_BYTE_SIZE);\
            const uint64_t new_hash = DENSITY_ALGORITHMS_MULTIPLY_SHIFT_64(new_unit, NEXT_HASH_BITS);\
            dictionary->entries[new_hash] = new_unit;\
            uint64_t *const bitmap = &dictionary->bitmap[new_hash >> 6];\
            const uint64_t mask = ((uint64_t) 1 << (new_hash & 0x3f));\
            const bool was_not_set = !(*bitmap & mask);\
            total_inserts += was_not_set;\
            *bitmap = *bitmap | mask;\
            transition_counter--;\
            SPECIAL_INSTRUCTIONS;\
        }\
        DENSITY_CHAMELEON_ENCODE_PUSH_SIGNATURE;\
    }

#define DENSITY_CHAMELEON_ENCODE_GENERATE_FAST_KERNEL(HASH_BITS, BYTE_GROUP_SIZE) \
    hits = 0;\
    inserts = 0;\
    collisions = 0;\
    while (DENSITY_LIKELY(in_position < in_limit)) {\
        samples_counter = 0x800;\
        while (DENSITY_LIKELY(in_position < in_limit && samples_counter--)) {\
            shift = 0;\
            for(uint_fast8_t unroll = 0; unroll < 9; unroll++) {\
                DENSITY_UNROLL_7(\
                    DENSITY_CHAMELEON_ENCODE_GENERATE_FAST_COMPRESSION_UNIT(BYTE_GROUP_SIZE, HASH_BITS, BYTE_GROUP_SIZE);\
                    shift ++;\
                );\
            }\
            DENSITY_CHAMELEON_ENCODE_GENERATE_STUDY_COMPRESSION_UNIT(HASH_BITS, BYTE_GROUP_SIZE);\
            DENSITY_CHAMELEON_ENCODE_PUSH_SIGNATURE;\
        }\
        if (collisions > hits) {\
            DENSITY_MEMSET(&dictionary->bitmap, 0, ((uint32_t) 1 << 8) >> 3);\
            DENSITY_ALGORITHMS_PRINT_SWITCH(HASH_BITS, BYTE_GROUP_SIZE, 8, 2);\
            DENSITY_CHAMELEON_ENCODE_GENERATE_TRANSITION_TO(HASH_BITS, BYTE_GROUP_SIZE, 8, 2, DENSITY_ALGORITHMS_TRANSITION_ROUNDS(8), );\
            goto study_kernel_8_2;\
        }\
        hits = 0;\
        inserts = 0;\
        collisions = 0;\
    }\
    goto finished;

#define DENSITY_CHAMELEON_ENCODE_GENERATE_STUDY_KERNEL(HASH_BITS, BYTE_GROUP_SIZE) \
    hits = 0;\
    inserts = 0;\
    collisions = 0;\
    stability = 0;\
    while (DENSITY_LIKELY(in_position < in_limit)) {\
        shift = 0;\
        for(uint_fast8_t unroll = 0; unroll < 4; unroll++) {\
            DENSITY_UNROLL_16(\
                DENSITY_CHAMELEON_ENCODE_GENERATE_STUDY_COMPRESSION_UNIT(HASH_BITS, BYTE_GROUP_SIZE);\
                shift ++;\
            );\
        }\
        DENSITY_CHAMELEON_ENCODE_PUSH_SIGNATURE;\
        if (DENSITY_UNLIKELY(!((hits + inserts + collisions) & (0x7ff)))) {\
            if (!inserts) {\
                if (total_inserts < (((uint64_t)1 << (HASH_BITS)) * 1) / 3 && (BYTE_GROUP_SIZE) <= 4) {\
                    DENSITY_MEMSET(&dictionary->bitmap, 0, ((uint32_t) 1 << (HASH_BITS)) >> 3);\
                    DENSITY_ALGORITHMS_PRINT_SWITCH(HASH_BITS, BYTE_GROUP_SIZE, HASH_BITS, DENSITY_ADD(BYTE_GROUP_SIZE,4));\
                    DENSITY_CHAMELEON_ENCODE_GENERATE_TRANSITION_TO(HASH_BITS, BYTE_GROUP_SIZE, HASH_BITS, DENSITY_ADD(BYTE_GROUP_SIZE,4), DENSITY_ALGORITHMS_TRANSITION_ROUNDS(HASH_BITS), );\
                    goto DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(study_kernel_,HASH_BITS),DENSITY_EVAL_CONCAT(_,DENSITY_ADD(BYTE_GROUP_SIZE,4)));\
                } else if (total_inserts < (((uint64_t)1 << (HASH_BITS)) * 2) / 3 && (BYTE_GROUP_SIZE) <= 6) {\
                    DENSITY_MEMSET(&dictionary->bitmap, 0, ((uint32_t) 1 << (HASH_BITS)) >> 3);\
                    DENSITY_ALGORITHMS_PRINT_SWITCH(HASH_BITS, BYTE_GROUP_SIZE, HASH_BITS, DENSITY_ADD(BYTE_GROUP_SIZE,2));\
                    DENSITY_CHAMELEON_ENCODE_GENERATE_TRANSITION_TO(HASH_BITS, BYTE_GROUP_SIZE, HASH_BITS, DENSITY_ADD(BYTE_GROUP_SIZE,2), DENSITY_ALGORITHMS_TRANSITION_ROUNDS(HASH_BITS), );\
                    goto DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(study_kernel_,HASH_BITS),DENSITY_EVAL_CONCAT(_,DENSITY_ADD(BYTE_GROUP_SIZE,2)));\
                }\
                if(++stability & ~0xf) {\
                    goto DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(fast_kernel_,HASH_BITS),DENSITY_EVAL_CONCAT(_,BYTE_GROUP_SIZE));\
                }\
            } else {\
                total_inserts += inserts;\
                if (total_inserts > ((((uint64_t)1 << (HASH_BITS)) * 15) >> 4)) {\
                    if ((HASH_BITS) < DENSITY_ALGORITHMS_MAX_DICTIONARY_BITS && (BYTE_GROUP_SIZE) <= 6) {\
                        DENSITY_MEMSET(&dictionary->bitmap, 0, ((uint32_t) 1 << (DENSITY_ADD(HASH_BITS,8))) >> 3);\
                        DENSITY_ALGORITHMS_PRINT_SWITCH(HASH_BITS, BYTE_GROUP_SIZE, DENSITY_ADD(HASH_BITS,8), DENSITY_ADD(BYTE_GROUP_SIZE,2));\
                        if(cleared) {\
                            DENSITY_CHAMELEON_ENCODE_GENERATE_TRANSITION_TO(HASH_BITS, BYTE_GROUP_SIZE, DENSITY_ADD(HASH_BITS,8), DENSITY_ADD(BYTE_GROUP_SIZE,2), DENSITY_ALGORITHMS_TRANSITION_ROUNDS(DENSITY_ADD(HASH_BITS,8)), );\
                            goto DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(study_kernel_,DENSITY_ADD(HASH_BITS,8)),DENSITY_EVAL_CONCAT(_,DENSITY_ADD(BYTE_GROUP_SIZE,2)));\
                        } else {\
                            DENSITY_CHAMELEON_ENCODE_GENERATE_TRANSITION_TO(HASH_BITS, BYTE_GROUP_SIZE, DENSITY_ADD(HASH_BITS,8), DENSITY_ADD(BYTE_GROUP_SIZE,2), DENSITY_ALGORITHMS_TRANSITION_ROUNDS(DENSITY_ADD(HASH_BITS,8)), DENSITY_CHAMELEON_ENCODE_CLEAR_DICTIONARY(DENSITY_ADD(HASH_BITS,8), DENSITY_ALGORITHMS_TRANSITION_ROUNDS(DENSITY_ADD(HASH_BITS,8))));\
                            cleared = true;\
                            goto DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(study_kernel_,DENSITY_ADD(HASH_BITS,8)),DENSITY_EVAL_CONCAT(_,DENSITY_ADD(BYTE_GROUP_SIZE,2)));\
                        }\
                    } else if (collisions > hits) {\
                        DENSITY_MEMSET(&dictionary->bitmap, 0, ((uint32_t) 1 << 8) >> 3);\
                        DENSITY_ALGORITHMS_PRINT_SWITCH(HASH_BITS, BYTE_GROUP_SIZE, 8, 2);\
                        DENSITY_CHAMELEON_ENCODE_GENERATE_TRANSITION_TO(HASH_BITS, BYTE_GROUP_SIZE, 8, 2, DENSITY_ALGORITHMS_TRANSITION_ROUNDS(8), );\
                        goto study_kernel_8_2;\
                    }\
                }\
                stability = 0;\
            }\
            hits = 0;\
            inserts = 0;\
            collisions = 0;\
        }\
    }\
    goto finished;

DENSITY_WINDOWS_EXPORT density_algorithm_exit_status density_chameleon_encode(density_algorithm_state *DENSITY_RESTRICT_DECLARE, const uint8_t **DENSITY_RESTRICT_DECLARE, uint_fast64_t, uint8_t **DENSITY_RESTRICT_DECLARE, uint_fast64_t);

#endif
