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
#define DENSITY_CHAMELEON_ENCODE_OUT_SAFE_DISTANCE(UNIT_NUMBER, BYTE_GROUP_SIZE) ((1 + (((UNIT_NUMBER) - 1) >> 6)) * sizeof(density_chameleon_signature) + (UNIT_NUMBER) * (BYTE_GROUP_SIZE))  // Maximum bytes written per unit processing

#define DENSITY_CHAMELEON_ENCODE_PREPARE_SIGNATURE \
    signature = 0;\
    signature_pointer = (density_chameleon_signature *) &out_array[out_position];\
    out_position += sizeof(density_chameleon_signature);

#define DENSITY_CHAMELEON_ENCODE_COPY_SIGNATURE \
    DENSITY_ENDIAN_CORRECT_BYTES_AND_FAST_MEMCPY(signature_pointer, &signature, 8);

#define DENSITY_CHAMELEON_ENCODE_PUSH_SIGNATURE \
    DENSITY_CHAMELEON_ENCODE_COPY_SIGNATURE;\
    DENSITY_CHAMELEON_ENCODE_PREPARE_SIGNATURE;

#define DENSITY_CHAMELEON_ENCODE_CLEAR_DICTIONARY(HASH_BITS, SPAN) \
    const uint_fast32_t step = ((((uint32_t)1 << (HASH_BITS)) - ((uint32_t)1 << ((HASH_BITS) - 8))) / (SPAN)) + 1;\
    const uint_fast32_t start = ((uint32_t)1 << ((HASH_BITS) - 8)) + transition_counter * step;\
    const uint_fast32_t end = DENSITY_MINIMUM(start + step, (uint32_t)1 << (HASH_BITS));\
    for(uint_fast32_t counter = start; counter < end; counter ++) {\
        const uint64_t bitmap = dictionary->bitmap[counter >> 6];\
        const uint64_t mask = ((uint64_t) 1 << (counter & 0x3f));\
        dictionary->entries[counter] = DENSITY_NOT_ZERO(bitmap & mask) * dictionary->entries[counter];\
    }

#define DENSITY_CHAMELEON_ENCODE_GENERATE_FAST_COMPRESSION_UNIT(HASH_BITS, BYTE_GROUP_SIZE) \
    DENSITY_ENDIAN_FAST_MEMCPY_AND_CORRECT_BYTES(&memcopy_64, &in_array[in_position], BYTE_GROUP_SIZE);\
    unit = DENSITY_ALGORITHMS_EXTRACT_64(memcopy_64, BYTE_GROUP_SIZE);\
    hash = DENSITY_ALGORITHMS_MULTIPLY_SHIFT_64(unit, HASH_BITS);\
    value = &dictionary->entries[hash];\
    switch (unit ^ *value) {\
        case 0:\
            signature |= ((uint64_t) DENSITY_CHAMELEON_SIGNATURE_FLAG_MAP << shift);\
            DENSITY_ENDIAN_CORRECT_BITS_AND_FAST_MEMCPY(&out_array[out_position], &hash, HASH_BITS);\
            out_position += ((HASH_BITS) >> 3);\
            break;\
        default:\
            *value = unit;\
            DENSITY_FAST_MEMCPY_BYTES(&out_array[out_position], &in_array[in_position], BYTE_GROUP_SIZE);\
            out_position += (BYTE_GROUP_SIZE);\
            break;\
    }\
    in_position += (BYTE_GROUP_SIZE);

#define DENSITY_CHAMELEON_ENCODE_GENERATE_STUDY_COMPRESSION_UNIT(HASH_BITS, BYTE_GROUP_SIZE) \
    DENSITY_ENDIAN_FAST_MEMCPY_AND_CORRECT_BYTES(&memcopy_64, &in_array[in_position], BYTE_GROUP_SIZE);\
    unit = DENSITY_ALGORITHMS_EXTRACT_64(memcopy_64, BYTE_GROUP_SIZE);\
    hash = DENSITY_ALGORITHMS_MULTIPLY_SHIFT_64(unit, HASH_BITS);\
    value = &dictionary->entries[hash];\
    switch (unit ^ *value) {\
        case 0:\
            hits++;\
            signature |= ((uint64_t) DENSITY_CHAMELEON_SIGNATURE_FLAG_MAP << shift);\
            DENSITY_ENDIAN_CORRECT_BITS_AND_FAST_MEMCPY(&out_array[out_position], &hash, HASH_BITS);\
            out_position += ((HASH_BITS) >> 3);\
            break;\
        default:\
            *value = unit;\
            uint64_t *const bitmap = &dictionary->bitmap[hash >> 6];\
            const uint64_t mask = ((uint64_t) 1 << (hash & 0x3f));\
            const bool was_not_set = !(*bitmap & mask);\
            inserts += was_not_set;\
            collisions += !was_not_set;\
            *bitmap = *bitmap | mask;\
            DENSITY_FAST_MEMCPY_BYTES(&out_array[out_position], &in_array[in_position], BYTE_GROUP_SIZE);\
            out_position += (BYTE_GROUP_SIZE);\
            break;\
    }\
    in_position += (BYTE_GROUP_SIZE);

#define DENSITY_CHAMELEON_ENCODE_GENERATE_TRANSITION_KERNEL_TEMPLATE(HASH_BITS, BYTE_GROUP_SIZE, NEXT_HASH_BITS, NEXT_BYTE_GROUP_SIZE, SPECIAL_LOOP_INSTRUCTIONS, SPECIAL_END_INSTRUCTIONS) \
    total_inserts = 0;\
    transition_counter = DENSITY_ALGORITHMS_TRANSITION_ROUNDS(NEXT_HASH_BITS);\
    in_limit = in_size - DENSITY_CHAMELEON_ENCODE_IN_SAFE_DISTANCE(64, BYTE_GROUP_SIZE);\
    out_limit = out_size - DENSITY_CHAMELEON_ENCODE_OUT_SAFE_DISTANCE(64, BYTE_GROUP_SIZE);\
    while (DENSITY_LIKELY(in_position <= in_limit && transition_counter)) {\
        for(shift = 0; shift < 0x40; shift++) {\
            DENSITY_CHAMELEON_ENCODE_GENERATE_FAST_COMPRESSION_UNIT(HASH_BITS, BYTE_GROUP_SIZE);\
            DENSITY_ENDIAN_FAST_MEMCPY_AND_CORRECT_BYTES(&memcopy_64, &in_array[in_position - (NEXT_BYTE_GROUP_SIZE)], NEXT_BYTE_GROUP_SIZE);\
            const uint64_t new_unit = DENSITY_ALGORITHMS_EXTRACT_64(memcopy_64, NEXT_BYTE_GROUP_SIZE);\
            const uint64_t new_hash = DENSITY_ALGORITHMS_MULTIPLY_SHIFT_64(new_unit, NEXT_HASH_BITS);\
            dictionary->entries[new_hash] = new_unit;\
            uint64_t *const bitmap = &dictionary->bitmap[new_hash >> 6];\
            const uint64_t mask = ((uint64_t) 1 << (new_hash & 0x3f));\
            const bool was_not_set = !(*bitmap & mask);\
            total_inserts += was_not_set;\
            *bitmap = *bitmap | mask;\
            transition_counter--;\
            SPECIAL_LOOP_INSTRUCTIONS;\
        }\
        DENSITY_CHAMELEON_ENCODE_PUSH_SIGNATURE;\
        if(DENSITY_UNLIKELY(out_position > out_limit))\
            goto DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(completion_kernel_,HASH_BITS),DENSITY_EVAL_CONCAT(_,BYTE_GROUP_SIZE));\
    }\
    ;SPECIAL_END_INSTRUCTIONS;\
    goto DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(study_kernel_,NEXT_HASH_BITS),DENSITY_EVAL_CONCAT(_,NEXT_BYTE_GROUP_SIZE));

#define DENSITY_CHAMELEON_ENCODE_GENERATE_TRANSITION_KERNEL(HASH_BITS, BYTE_GROUP_SIZE, NEXT_HASH_BITS, NEXT_BYTE_GROUP_SIZE) \
DENSITY_CHAMELEON_ENCODE_GENERATE_TRANSITION_KERNEL_TEMPLATE(HASH_BITS, BYTE_GROUP_SIZE, NEXT_HASH_BITS, NEXT_BYTE_GROUP_SIZE,,);

#define DENSITY_CHAMELEON_ENCODE_GENERATE_TRANSITION_INIT_KERNEL(HASH_BITS, BYTE_GROUP_SIZE, NEXT_HASH_BITS, NEXT_BYTE_GROUP_SIZE) \
DENSITY_CHAMELEON_ENCODE_GENERATE_TRANSITION_KERNEL_TEMPLATE(HASH_BITS, BYTE_GROUP_SIZE, NEXT_HASH_BITS, NEXT_BYTE_GROUP_SIZE,DENSITY_CHAMELEON_ENCODE_CLEAR_DICTIONARY(NEXT_HASH_BITS, DENSITY_ALGORITHMS_TRANSITION_ROUNDS(NEXT_HASH_BITS)),cleared = true);

#define DENSITY_CHAMELEON_ENCODE_GENERATE_FAST_KERNEL(HASH_BITS, BYTE_GROUP_SIZE) \
    hits = 0;\
    inserts = 0;\
    collisions = 0;\
    in_limit = in_size - DENSITY_CHAMELEON_ENCODE_IN_SAFE_DISTANCE(64, BYTE_GROUP_SIZE);\
    out_limit = out_size - DENSITY_CHAMELEON_ENCODE_OUT_SAFE_DISTANCE(64, BYTE_GROUP_SIZE);\
    while (DENSITY_LIKELY(in_position <= in_limit)) {\
        samples_counter = 0x800;\
        while (DENSITY_LIKELY(in_position <= in_limit && samples_counter--)) {\
            shift = 0;\
            for(uint_fast8_t unroll = 0; unroll < 9; unroll++) {\
                DENSITY_PREFETCH(&in_array[in_position + DENSITY_CHAMELEON_ENCODE_IN_SAFE_DISTANCE(4, BYTE_GROUP_SIZE)], 0, 0);\
                DENSITY_UNROLL_7(\
                    DENSITY_CHAMELEON_ENCODE_GENERATE_FAST_COMPRESSION_UNIT(HASH_BITS, BYTE_GROUP_SIZE);\
                    shift ++;\
                );\
            }\
            DENSITY_CHAMELEON_ENCODE_GENERATE_STUDY_COMPRESSION_UNIT(HASH_BITS, BYTE_GROUP_SIZE);\
            DENSITY_CHAMELEON_ENCODE_PUSH_SIGNATURE;\
            if(DENSITY_UNLIKELY(out_position > out_limit))\
                goto DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(completion_kernel_,HASH_BITS),DENSITY_EVAL_CONCAT(_,BYTE_GROUP_SIZE));\
        }\
        if (collisions > hits) {\
            DENSITY_MEMSET(&dictionary->bitmap, 0, ((uint32_t) 1 << 8) >> 3);\
            goto study_kernel_8_2; /* No transition here as the current dictionary is inefficient */\
        }\
        hits = 0;\
        inserts = 0;\
        collisions = 0;\
    }\
    goto DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(completion_kernel_,HASH_BITS),DENSITY_EVAL_CONCAT(_,BYTE_GROUP_SIZE));

#define DENSITY_CHAMELEON_ENCODE_GENERATE_STUDY_KERNEL(HASH_BITS, BYTE_GROUP_SIZE) \
    hits = 0;\
    inserts = 0;\
    collisions = 0;\
    stability = 0;\
    in_limit = in_size - DENSITY_CHAMELEON_ENCODE_IN_SAFE_DISTANCE(64, BYTE_GROUP_SIZE);\
    out_limit = out_size - DENSITY_CHAMELEON_ENCODE_OUT_SAFE_DISTANCE(64, BYTE_GROUP_SIZE);\
    while (DENSITY_LIKELY(in_position <= in_limit)) {\
        shift = 0;\
        for(uint_fast8_t unroll = 0; unroll < 8; unroll++) {\
            DENSITY_PREFETCH(&in_array[in_position + DENSITY_CHAMELEON_ENCODE_IN_SAFE_DISTANCE(4, BYTE_GROUP_SIZE)], 0, 0);\
            DENSITY_UNROLL_8(\
                DENSITY_CHAMELEON_ENCODE_GENERATE_STUDY_COMPRESSION_UNIT(HASH_BITS, BYTE_GROUP_SIZE);\
                shift ++;\
            );\
        }\
        DENSITY_CHAMELEON_ENCODE_PUSH_SIGNATURE;\
        if(DENSITY_UNLIKELY(out_position > out_limit))\
            goto DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(completion_kernel_,HASH_BITS),DENSITY_EVAL_CONCAT(_,BYTE_GROUP_SIZE));\
        if (DENSITY_UNLIKELY(!((hits + inserts + collisions) & (0x7ff)))) {\
            if (!inserts) {\
                if (total_inserts < (((uint64_t)1 << (HASH_BITS)) * 2) / 3 && (BYTE_GROUP_SIZE) <= 6) {\
                    DENSITY_MEMSET(&dictionary->bitmap, 0, ((uint32_t) 1 << (HASH_BITS)) >> 3);\
                    if (total_inserts < (((uint64_t)1 << (HASH_BITS)) * 1) / 3 && (BYTE_GROUP_SIZE) <= 4) {\
                        goto DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(transition_kernel_,HASH_BITS),DENSITY_EVAL_CONCAT(_,BYTE_GROUP_SIZE)),DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(_,HASH_BITS),DENSITY_EVAL_CONCAT(_,DENSITY_ADD(BYTE_GROUP_SIZE,4))));\
                    } else {\
                        goto DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(transition_kernel_,HASH_BITS),DENSITY_EVAL_CONCAT(_,BYTE_GROUP_SIZE)),DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(_,HASH_BITS),DENSITY_EVAL_CONCAT(_,DENSITY_ADD(BYTE_GROUP_SIZE,2))));\
                    }\
                }\
                if(++stability & ~0xf) {\
                    goto DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(fast_kernel_,HASH_BITS),DENSITY_EVAL_CONCAT(_,BYTE_GROUP_SIZE));\
                }\
            } else {\
                total_inserts += inserts;\
                if (total_inserts > ((((uint64_t)1 << (HASH_BITS)) * 15) >> 4)) {\
                    if ((HASH_BITS) < DENSITY_ALGORITHMS_MAX_DICTIONARY_BITS && (BYTE_GROUP_SIZE) <= 6) {\
                        DENSITY_MEMSET(&dictionary->bitmap, 0, ((uint32_t) 1 << (DENSITY_ADD(HASH_BITS,8))) >> 3);\
                        if(cleared) {\
                            goto DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(transition_kernel_,HASH_BITS),DENSITY_EVAL_CONCAT(_,BYTE_GROUP_SIZE)),DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(_,DENSITY_ADD(HASH_BITS,8)),DENSITY_EVAL_CONCAT(_,DENSITY_ADD(BYTE_GROUP_SIZE,2))));\
                        } else {\
                            goto DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(transition_init_kernel_,HASH_BITS),DENSITY_EVAL_CONCAT(_,BYTE_GROUP_SIZE)),DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(_,DENSITY_ADD(HASH_BITS,8)),DENSITY_EVAL_CONCAT(_,DENSITY_ADD(BYTE_GROUP_SIZE,2))));\
                        }\
                    } else if (collisions > hits) {\
                        DENSITY_MEMSET(&dictionary->bitmap, 0, ((uint32_t) 1 << 8) >> 3);\
                        goto study_kernel_8_2; /* No transition here as the current dictionary is inefficient */\
                    }\
                }\
                stability = 0;\
            }\
            hits = 0;\
            inserts = 0;\
            collisions = 0;\
        }\
    }\
    goto DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(completion_kernel_,HASH_BITS),DENSITY_EVAL_CONCAT(_,BYTE_GROUP_SIZE));

#define DENSITY_CHAMELEON_ENCODE_GENERATE_COMPLETION_KERNEL(HASH_BITS, BYTE_GROUP_SIZE) \
    in_limit = in_size - DENSITY_CHAMELEON_ENCODE_IN_SAFE_DISTANCE(1, BYTE_GROUP_SIZE);\
    out_limit = out_size - DENSITY_CHAMELEON_ENCODE_OUT_SAFE_DISTANCE(1, BYTE_GROUP_SIZE);\
    shift = 0;\
    while (in_position <= in_limit) {\
        DENSITY_CHAMELEON_ENCODE_GENERATE_FAST_COMPRESSION_UNIT(HASH_BITS, BYTE_GROUP_SIZE);\
        shift ++;\
        if(DENSITY_UNLIKELY(!(shift & 0x3f))) {\
            DENSITY_CHAMELEON_ENCODE_PUSH_SIGNATURE;\
            shift = 0;\
        }\
        if(out_position > out_limit)\
            goto output_stall;\
    }\
    signature |= ((uint64_t) DENSITY_CHAMELEON_SIGNATURE_FLAG_CHUNK << shift);  /*End marker*/\
    { /*Avoid local variable redefinition */\
        DENSITY_CHAMELEON_ENCODE_COPY_SIGNATURE;\
    }\
    goto finish;

DENSITY_WINDOWS_EXPORT density_algorithm_exit_status density_chameleon_encode(density_algorithm_state *DENSITY_RESTRICT_DECLARE, const uint8_t **DENSITY_RESTRICT_DECLARE, uint_fast64_t, uint8_t **DENSITY_RESTRICT_DECLARE, uint_fast64_t);

#endif
