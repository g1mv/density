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

#include "../../algorithms.h"
#include "../dictionary/chameleon_dictionary.h"

#define DENSITY_CHAMELEON_ENCODE_IN_SAFE_DISTANCE(UNIT_NUMBER, BYTE_GROUP_SIZE) ((UNIT_NUMBER) * (BYTE_GROUP_SIZE))  // Maximum bytes read per unit processing

#define DENSITY_CHAMELEON_ENCODE_PREPARE_SIGNATURE \
    signature = 0;\
    signature_pointer = (density_chameleon_signature *) &out_array[out_position];\
    out_position += sizeof(density_chameleon_signature);

#define DENSITY_CHAMELEON_ENCODE_COPY_SIGNATURE(COPY_FUNCTION) \
    DENSITY_ENDIAN_CORRECT_64_AND_MEMCPY(COPY_FUNCTION, signature_pointer, &signature, 8);

#define DENSITY_CHAMELEON_ENCODE_CLEAR_DICTIONARY \
    /* 16-bit dictionary clear */\
    const uint_fast32_t pivot = (transition_counter << 6) + (1 << 8);\
    const uint64_t dictionary_bitmap = dictionary->bitmap[pivot >> 6];\
    uint64_t *const array = &dictionary->entries[pivot];\
    for(uint_fast8_t clear_index = 0x40; DENSITY_LIKELY(clear_index);) {\
        const uint64_t mask = ((uint64_t) 1 << (--clear_index & 0x3f));\
        array[clear_index] = DENSITY_NOT_ZERO(dictionary_bitmap & mask) * array[clear_index];\
    }

#define DENSITY_CHAMELEON_ENCODE_COMPUTING_STAGE(COPY_FUNCTION, POSITION, HASH_BYTES, BYTE_GROUP_SIZE) \
    DENSITY_ENDIAN_MEMCPY_AND_CORRECT_64(COPY_FUNCTION, &memcopy_64, &in_array[POSITION], BYTE_GROUP_SIZE);\
    unit = DENSITY_ALGORITHMS_EXTRACT_64(memcopy_64, BYTE_GROUP_SIZE);\
    hash = DENSITY_ALGORITHMS_MULTIPLY_SHIFT_64(unit, HASH_BYTES);\
    value = &dictionary->entries[hash];

#define DENSITY_CHAMELEON_ENCODE_GENERATE_FAST_COMPRESSION_UNIT(COPY_FUNCTION, HASH_BYTES, BYTE_GROUP_SIZE) \
    DENSITY_CHAMELEON_ENCODE_COMPUTING_STAGE(COPY_FUNCTION, in_position, HASH_BYTES, BYTE_GROUP_SIZE);\
    switch (unit ^ *value) {\
        case 0:\
            signature |= ((uint64_t) DENSITY_CHAMELEON_SIGNATURE_FLAG_MAP << shift);\
            DENSITY_ENDIAN_CORRECT_64_AND_MEMCPY(COPY_FUNCTION, &out_array[out_position], &hash, HASH_BYTES);\
            out_position += (HASH_BYTES);\
            break;\
        default:\
            *value = unit;\
            COPY_FUNCTION(&out_array[out_position], &in_array[in_position], BYTE_GROUP_SIZE);\
            out_position += (BYTE_GROUP_SIZE);\
            break;\
    }\
    in_position += (BYTE_GROUP_SIZE);

#define DENSITY_CHAMELEON_ENCODE_GENERATE_STUDY_COMPRESSION_UNIT(COPY_FUNCTION, HASH_BYTES, BYTE_GROUP_SIZE) \
    DENSITY_CHAMELEON_ENCODE_COMPUTING_STAGE(COPY_FUNCTION, in_position, HASH_BYTES, BYTE_GROUP_SIZE);\
    switch (unit ^ *value) {\
        case 0:\
            hits++;\
            signature |= ((uint64_t) DENSITY_CHAMELEON_SIGNATURE_FLAG_MAP << shift);\
            DENSITY_ENDIAN_CORRECT_64_AND_MEMCPY(COPY_FUNCTION, &out_array[out_position], &hash, HASH_BYTES);\
            out_position += (HASH_BYTES);\
            break;\
        default:\
            *value = unit;\
            uint64_t *const bitmap = &dictionary->bitmap[hash >> 6];\
            const uint64_t mask = ((uint64_t) 1 << (hash & 0x3f));\
            const bool was_not_set = !(*bitmap & mask);\
            inserts += was_not_set;\
            collisions += !was_not_set;\
            *bitmap = *bitmap | mask;\
            COPY_FUNCTION(&out_array[out_position], &in_array[in_position], BYTE_GROUP_SIZE);\
            out_position += (BYTE_GROUP_SIZE);\
            break;\
    }\
    in_position += (BYTE_GROUP_SIZE);

#define DENSITY_CHAMELEON_ENCODE_GENERATE_TRANSITION_KERNEL_TEMPLATE(HASH_BYTES, BYTE_GROUP_SIZE, NEXT_HASH_BYTES, NEXT_BYTE_GROUP_SIZE, SPECIAL_LOOP_INSTRUCTIONS, SPECIAL_END_INSTRUCTIONS) \
    DENSITY_ALGORITHM_READ_PROTECTION(DENSITY_CHAMELEON_ENCODE_IN_SAFE_DISTANCE(64, BYTE_GROUP_SIZE) + DENSITY_FAST_MEMCPY_EXTRA_BYTES(BYTE_GROUP_SIZE), DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(completion_kernel_,HASH_BYTES),DENSITY_EVAL_CONCAT(_,BYTE_GROUP_SIZE)));\
    total_inserts = 0;\
    transition_counter = DENSITY_ALGORITHMS_TRANSITION_ROUNDS(HASH_BYTES, NEXT_HASH_BYTES);\
    while (DENSITY_LIKELY(transition_counter)) {\
        if(DENSITY_UNLIKELY(in_position > in_limit))\
            goto DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(completion_kernel_,HASH_BYTES),DENSITY_EVAL_CONCAT(_,BYTE_GROUP_SIZE));\
        DENSITY_CHAMELEON_ENCODE_PREPARE_SIGNATURE;\
        for(shift = 0; DENSITY_LIKELY(shift < 64 && transition_counter);) {\
            for(uint_fast8_t unroll = DENSITY_ALGORITHMS_TRANSITION_UNROLL; unroll; unroll--) {\
                DENSITY_CHAMELEON_ENCODE_GENERATE_FAST_COMPRESSION_UNIT(DENSITY_FAST_MEMCPY, HASH_BYTES, BYTE_GROUP_SIZE);\
                DENSITY_CHAMELEON_ENCODE_COMPUTING_STAGE(DENSITY_FAST_MEMCPY, in_position - (NEXT_BYTE_GROUP_SIZE), NEXT_HASH_BYTES, NEXT_BYTE_GROUP_SIZE);\
                *value = unit;\
                uint64_t *const bitmap = &dictionary->bitmap[hash >> 6];\
                const uint64_t mask = ((uint64_t) 1 << (hash & 0x3f));\
                const bool was_not_set = !(*bitmap & mask);\
                total_inserts += was_not_set;\
                *bitmap = *bitmap | mask;\
                shift ++;\
            };\
            transition_counter--;\
            SPECIAL_LOOP_INSTRUCTIONS;\
        };\
        DENSITY_CHAMELEON_ENCODE_COPY_SIGNATURE(DENSITY_FAST_MEMCPY);\
    }\
    ;SPECIAL_END_INSTRUCTIONS;\
    goto DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(study_kernel_,NEXT_HASH_BYTES),DENSITY_EVAL_CONCAT(_,NEXT_BYTE_GROUP_SIZE));

#define DENSITY_CHAMELEON_ENCODE_GENERATE_TRANSITION_KERNEL(HASH_BYTES, BYTE_GROUP_SIZE, NEXT_HASH_BYTES, NEXT_BYTE_GROUP_SIZE) \
DENSITY_CHAMELEON_ENCODE_GENERATE_TRANSITION_KERNEL_TEMPLATE(HASH_BYTES, BYTE_GROUP_SIZE, NEXT_HASH_BYTES, NEXT_BYTE_GROUP_SIZE,,);

#define DENSITY_CHAMELEON_ENCODE_GENERATE_TRANSITION_INIT_KERNEL(HASH_BYTES, BYTE_GROUP_SIZE, NEXT_HASH_BYTES, NEXT_BYTE_GROUP_SIZE) \
DENSITY_CHAMELEON_ENCODE_GENERATE_TRANSITION_KERNEL_TEMPLATE(HASH_BYTES, BYTE_GROUP_SIZE, NEXT_HASH_BYTES, NEXT_BYTE_GROUP_SIZE,DENSITY_CHAMELEON_ENCODE_CLEAR_DICTIONARY,state->dictionary_cleared = true);

#ifdef __clang__
#define DENSITY_CHAMELEON_ENCODE_FAST_KERNEL_INNER_LOOP(HASH_BYTES, BYTE_GROUP_SIZE) \
    DENSITY_UNROLL_3(\
    DENSITY_PREFETCH(&in_array[in_position + 64], 0, 0);\
    for(uint_fast8_t unroll = 16; DENSITY_LIKELY(unroll); unroll--) {\
        DENSITY_CHAMELEON_ENCODE_GENERATE_FAST_COMPRESSION_UNIT(DENSITY_FAST_MEMCPY, HASH_BYTES, BYTE_GROUP_SIZE);\
        shift ++;\
    }\
    );\
    DENSITY_PREFETCH(&in_array[in_position + 64], 0, 0);\
    for(uint_fast8_t unroll = 15; DENSITY_LIKELY(unroll); unroll--) {\
        DENSITY_CHAMELEON_ENCODE_GENERATE_FAST_COMPRESSION_UNIT(DENSITY_FAST_MEMCPY, HASH_BYTES, BYTE_GROUP_SIZE);\
        shift ++;\
    }
#else
#define DENSITY_CHAMELEON_ENCODE_FAST_KERNEL_INNER_LOOP(HASH_BYTES, BYTE_GROUP_SIZE) \
    for(uint_fast8_t unroll = 9; DENSITY_LIKELY(unroll); unroll--) {\
        DENSITY_PREFETCH(&in_array[in_position + 64], 0, 0);\
        DENSITY_UNROLL_7(\
            DENSITY_CHAMELEON_ENCODE_GENERATE_FAST_COMPRESSION_UNIT(DENSITY_FAST_MEMCPY, HASH_BYTES, BYTE_GROUP_SIZE);\
            shift ++;\
        );\
    }
#endif

#define DENSITY_CHAMELEON_ENCODE_GENERATE_FAST_KERNEL(HASH_BYTES, BYTE_GROUP_SIZE) \
    DENSITY_ALGORITHM_READ_PROTECTION(DENSITY_CHAMELEON_ENCODE_IN_SAFE_DISTANCE(64, BYTE_GROUP_SIZE) + DENSITY_FAST_MEMCPY_EXTRA_BYTES(BYTE_GROUP_SIZE), DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(completion_kernel_,HASH_BYTES),DENSITY_EVAL_CONCAT(_,BYTE_GROUP_SIZE)));\
    hits = 0;\
    inserts = 0;\
    collisions = 0;\
    for (;;) {\
        samples_counter = 0x800;\
        while (DENSITY_LIKELY(samples_counter--)) {\
            if(DENSITY_UNLIKELY(in_position > in_limit))\
                goto DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(completion_kernel_,HASH_BYTES),DENSITY_EVAL_CONCAT(_,BYTE_GROUP_SIZE));\
            DENSITY_CHAMELEON_ENCODE_PREPARE_SIGNATURE;\
            shift = 0;\
            DENSITY_CHAMELEON_ENCODE_FAST_KERNEL_INNER_LOOP(HASH_BYTES, BYTE_GROUP_SIZE);\
            DENSITY_CHAMELEON_ENCODE_GENERATE_STUDY_COMPRESSION_UNIT(DENSITY_FAST_MEMCPY, HASH_BYTES, BYTE_GROUP_SIZE);\
            DENSITY_CHAMELEON_ENCODE_COPY_SIGNATURE(DENSITY_FAST_MEMCPY);\
        }\
        /*printf("%lld / %lld\n", collisions, hits);*/\
        if ((collisions << 1) > hits) {\
            DENSITY_MEMSET(&dictionary->bitmap, 0, ((uint32_t) 1 << 8) >> 3);\
            goto study_kernel_1_2; /* No transition here as the current dictionary is inefficient */\
        }\
        hits = 0;\
        inserts = 0;\
        collisions = 0;\
    }\
    goto DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(completion_kernel_,HASH_BYTES),DENSITY_EVAL_CONCAT(_,BYTE_GROUP_SIZE));

#define DENSITY_CHAMELEON_ENCODE_GENERATE_STUDY_KERNEL(HASH_BYTES, BYTE_GROUP_SIZE, INCOMPRESSIBLE_PROTECTION_FUNCTION_START, INCOMPRESSIBLE_PROTECTION_FUNCTION_END) \
    /*printf("[%lu,%lu] %llu - %llu/%llu\n", HASH_BYTES, BYTE_GROUP_SIZE, in_position, collisions, hits);*/\
    DENSITY_ALGORITHM_READ_PROTECTION(DENSITY_CHAMELEON_ENCODE_IN_SAFE_DISTANCE(64, BYTE_GROUP_SIZE) + DENSITY_FAST_MEMCPY_EXTRA_BYTES(BYTE_GROUP_SIZE), DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(completion_kernel_,HASH_BYTES),DENSITY_EVAL_CONCAT(_,BYTE_GROUP_SIZE)));\
    hits = 0;\
    inserts = 0;\
    collisions = 0;\
    stability = 0;\
    while (DENSITY_LIKELY(in_position <= in_limit)) {\
        INCOMPRESSIBLE_PROTECTION_FUNCTION_START(out_position, 64 * (BYTE_GROUP_SIZE));\
        DENSITY_CHAMELEON_ENCODE_PREPARE_SIGNATURE;\
        shift = 0;\
        for(uint_fast8_t unroll = 0; unroll < 8; unroll++) {\
            DENSITY_PREFETCH(&in_array[in_position + 64], 0, 0);\
            DENSITY_UNROLL_8(\
                DENSITY_CHAMELEON_ENCODE_GENERATE_STUDY_COMPRESSION_UNIT(DENSITY_FAST_MEMCPY, HASH_BYTES, BYTE_GROUP_SIZE);\
                shift ++;\
            );\
        }\
        DENSITY_CHAMELEON_ENCODE_COPY_SIGNATURE(DENSITY_FAST_MEMCPY);\
        INCOMPRESSIBLE_PROTECTION_FUNCTION_END(out_position, 64 * (BYTE_GROUP_SIZE));\
        if (DENSITY_UNLIKELY(!((hits + inserts + collisions) & (0x1ff)))) {\
            if (!inserts) {\
                if (total_inserts < (((uint64_t)1 << ((HASH_BYTES) << 3)) * 2) / 3 && (BYTE_GROUP_SIZE) <= 6) {\
                    if(in_position + DENSITY_ALGORITHMS_TRANSITION_ROUNDS(HASH_BYTES, DENSITY_ADD(HASH_BYTES,1)) * DENSITY_ALGORITHMS_TRANSITION_UNROLL * (BYTE_GROUP_SIZE) < in_size) {\
                        DENSITY_MEMSET(&dictionary->bitmap, 0, ((uint32_t) 1 << ((HASH_BYTES) << 3)) >> 3);\
                        if (total_inserts < (((uint64_t)1 << ((HASH_BYTES) << 3)) * 1) / 3 && (BYTE_GROUP_SIZE) <= 4) {\
                            goto DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(transition_kernel_,HASH_BYTES),DENSITY_EVAL_CONCAT(_,BYTE_GROUP_SIZE)),DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(_,HASH_BYTES),DENSITY_EVAL_CONCAT(_,DENSITY_ADD(BYTE_GROUP_SIZE,4))));\
                        } else {\
                            goto DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(transition_kernel_,HASH_BYTES),DENSITY_EVAL_CONCAT(_,BYTE_GROUP_SIZE)),DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(_,HASH_BYTES),DENSITY_EVAL_CONCAT(_,DENSITY_ADD(BYTE_GROUP_SIZE,2))));\
                        }\
                    } else { /* Not enough input bytes left for transition to finish, use fast kernel instead */\
                        goto DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(fast_kernel_,HASH_BYTES),DENSITY_EVAL_CONCAT(_,BYTE_GROUP_SIZE));\
                    }\
                }\
                if(++stability & ~0xf) {\
                    goto DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(fast_kernel_,HASH_BYTES),DENSITY_EVAL_CONCAT(_,BYTE_GROUP_SIZE));\
                }\
            } else {\
                total_inserts += inserts;\
                if (total_inserts > ((((uint64_t)1 << ((HASH_BYTES) << 3)) * 15) >> 4)) {\
                    if (((HASH_BYTES) << 3) < DENSITY_ALGORITHMS_MAX_DICTIONARY_KEY_BITS && (BYTE_GROUP_SIZE) <= 6) {\
                        if(in_position + DENSITY_ALGORITHMS_TRANSITION_ROUNDS(HASH_BYTES, DENSITY_ADD(HASH_BYTES,1)) * DENSITY_ALGORITHMS_TRANSITION_UNROLL * (BYTE_GROUP_SIZE) < in_size) {\
                            DENSITY_FAST_CLEAR_ARRAY_64(dictionary->bitmap, ((uint32_t) 1 << (DENSITY_ADD(HASH_BYTES,1) << 3)) >> 6);\
                            if(state->dictionary_cleared) {\
                                goto DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(transition_kernel_,HASH_BYTES),DENSITY_EVAL_CONCAT(_,BYTE_GROUP_SIZE)),DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(_,DENSITY_ADD(HASH_BYTES,1)),DENSITY_EVAL_CONCAT(_,DENSITY_ADD(BYTE_GROUP_SIZE,2))));\
                            } else {\
                                goto DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(transition_init_kernel_,HASH_BYTES),DENSITY_EVAL_CONCAT(_,BYTE_GROUP_SIZE)),DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(_,DENSITY_ADD(HASH_BYTES,1)),DENSITY_EVAL_CONCAT(_,DENSITY_ADD(BYTE_GROUP_SIZE,2))));\
                            }\
                        } else { /* Not enough input bytes left for transition to finish, use fast kernel instead */\
                            goto DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(fast_kernel_,HASH_BYTES),DENSITY_EVAL_CONCAT(_,BYTE_GROUP_SIZE));\
                        }\
                    } else if (collisions > hits) {\
                        DENSITY_MEMSET(&dictionary->bitmap, 0, ((uint32_t) 1 << 8) >> 3);\
                        goto study_kernel_1_2; /* No transition here as the current dictionary is inefficient */\
                    }\
                }\
                stability = 0;\
            }\
            hits = 0;\
            inserts = 0;\
            collisions = 0;\
        }\
    }\
    goto DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(completion_kernel_,HASH_BYTES),DENSITY_EVAL_CONCAT(_,BYTE_GROUP_SIZE));

#define DENSITY_CHAMELEON_ENCODE_GENERATE_COMPLETION_KERNEL(HASH_BYTES, BYTE_GROUP_SIZE) \
    DENSITY_ALGORITHM_READ_PROTECTION(DENSITY_CHAMELEON_ENCODE_IN_SAFE_DISTANCE(1, BYTE_GROUP_SIZE), finish);\
    DENSITY_CHAMELEON_ENCODE_PREPARE_SIGNATURE;\
    shift = 0;\
    while (in_position <= in_limit) {\
        DENSITY_CHAMELEON_ENCODE_GENERATE_FAST_COMPRESSION_UNIT(DENSITY_MEMCPY, HASH_BYTES, BYTE_GROUP_SIZE);\
        shift ++;\
        if(DENSITY_UNLIKELY(!(shift & 0x3f))) {\
            DENSITY_CHAMELEON_ENCODE_COPY_SIGNATURE(DENSITY_MEMCPY);\
            DENSITY_CHAMELEON_ENCODE_PREPARE_SIGNATURE;\
            shift = 0;\
        }\
    }\
    signature |= ((uint64_t) DENSITY_CHAMELEON_SIGNATURE_FLAG_CHUNK << shift); /*End marker*/\
    { /* Avoid local variable redefinition */\
        DENSITY_CHAMELEON_ENCODE_COPY_SIGNATURE(DENSITY_MEMCPY);\
    }\
    goto finish;

DENSITY_WINDOWS_EXPORT density_algorithm_exit_status density_chameleon_encode(density_algorithm_state *DENSITY_RESTRICT_DECLARE, const uint8_t **DENSITY_RESTRICT_DECLARE, uint_fast64_t, uint8_t **DENSITY_RESTRICT_DECLARE, uint_fast64_t);

#endif
