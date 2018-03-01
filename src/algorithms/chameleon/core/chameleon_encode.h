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

#define DENSITY_CHAMELEON_ENCODE_PREPARE_SIGNATURE(OUT, SIGNATURE, SIGNATURE_POINTER) \
    (SIGNATURE) = 0;\
    (SIGNATURE_POINTER) = (density_chameleon_signature *) (OUT);\
    (OUT) += sizeof(density_chameleon_signature);

#define DENSITY_CHAMELEON_PUSH_SIGNATURE(OUT, SIGNATURE, SIGNATURE_POINTER) \
    DENSITY_MEMCPY(SIGNATURE_POINTER, &(SIGNATURE), sizeof(density_chameleon_signature));\
    DENSITY_CHAMELEON_ENCODE_PREPARE_SIGNATURE(OUT, SIGNATURE, SIGNATURE_POINTER);

#define DENSITY_CHAMELEON_ENCODE_CLEAR_DICTIONARY(HASH_BITS, SPAN, DICTIONARY, TRANSITION_COUNTER) \
    const uint_fast32_t step = ((((uint32_t)1 << (HASH_BITS)) - 256) / (SPAN)) + 1;\
    const uint_fast32_t start = ((uint32_t)1 << 8) + (TRANSITION_COUNTER) * step;\
    const uint_fast32_t end = DENSITY_MINIMUM(start + step, (uint32_t)1 << (HASH_BITS));\
    DENSITY_ALGORITHMS_PRINT_CLEAR(start, end);\
    for(uint_fast32_t counter = start; counter < end; counter ++) {\
        const uint64_t bitmap = (DICTIONARY)->bitmap[counter >> 6];\
        const uint64_t mask = ((uint64_t) 1 << (counter & 0x3f));\
        (DICTIONARY)->entries[counter] = DENSITY_NOT_ZERO(bitmap & mask) * (DICTIONARY)->entries[counter];\
    }

#define DENSITY_CHAMELEON_ENCODE_GENERATE_TRANSITION_TO(IN, IN_LIMIT, OUT, OUT_LIMIT, HASH_BITS, BYTE_GROUP_SIZE, NEXT_HASH_BITS, NEXT_GROUP_BYTE_SIZE, DICTIONARY, SPAN, MEMCOPY, TRANSITION_COUNTER, TOTAL_INSERTS, SIGNATURE, SIGNATURE_POINTER, SPECIAL_INSTRUCTIONS) \
    DENSITY_ALGORITHMS_PRINT_TRANSITION(HASH_BITS, BYTE_GROUP_SIZE, NEXT_HASH_BITS, NEXT_GROUP_BYTE_SIZE, SPAN)\
    (TOTAL_INSERTS) = 0;\
    (TRANSITION_COUNTER) = SPAN;\
    while (DENSITY_LIKELY((IN) < (IN_LIMIT) && (TRANSITION_COUNTER))) {\
        for(uint_fast8_t shift = 0; shift < 0x40; shift++) {\
            DENSITY_MEMCPY(&(MEMCOPY), IN, DENSITY_MAXIMUM(BYTE_GROUP_SIZE, NEXT_GROUP_BYTE_SIZE));\
            const uint64_t unit = DENSITY_ALGORITHMS_EXTRACT_64(MEMCOPY, BYTE_GROUP_SIZE);\
            const uint64_t new_unit = DENSITY_ALGORITHMS_EXTRACT_64(MEMCOPY, NEXT_GROUP_BYTE_SIZE);\
            const uint64_t hash = DENSITY_ALGORITHMS_MULTIPLY_SHIFT_64(unit, HASH_BITS);\
            const uint64_t new_hash = DENSITY_ALGORITHMS_MULTIPLY_SHIFT_64(new_unit, NEXT_HASH_BITS);\
            (IN) += (BYTE_GROUP_SIZE);\
            uint64_t *const value = &(DICTIONARY)->entries[hash];\
            switch (unit ^ *value) {\
                case 0:\
                    (SIGNATURE) |= ((uint64_t) DENSITY_CHAMELEON_SIGNATURE_FLAG_MAP << shift);\
                    DENSITY_MEMCPY(OUT, &hash, (HASH_BITS) >> 3);\
                    (OUT) += ((HASH_BITS) >> 3);\
                    break;\
                default:\
                    *value = unit;\
                    DENSITY_MEMCPY(OUT, &unit, BYTE_GROUP_SIZE);\
                    (OUT) += BYTE_GROUP_SIZE;\
                    break;\
            }\
            (DICTIONARY)->entries[new_hash] = new_unit;\
            uint64_t *const bitmap = &(DICTIONARY)->bitmap[new_hash >> 6];\
            const uint64_t mask = ((uint64_t) 1 << (new_hash & 0x3f));\
            const bool was_not_set = !(*bitmap & mask);\
            (TOTAL_INSERTS) += was_not_set;\
            *bitmap = *bitmap | mask;\
            (TRANSITION_COUNTER)--;\
            SPECIAL_INSTRUCTIONS;\
        }\
        DENSITY_CHAMELEON_PUSH_SIGNATURE(OUT, SIGNATURE, SIGNATURE_POINTER);\
    }

#define DENSITY_CHAMELEON_ENCODE_GENERATE_KERNEL(IN, IN_LIMIT, OUT, OUT_LIMIT, HASH_BITS, BYTE_GROUP_SIZE, DICTIONARY, DICTIONARY_CLEAR, MEMCOPY, HITS, INSERTS, TOTAL_INSERTS, COLLISIONS, TRANSITION_COUNTER, SIGNATURE, SIGNATURE_POINTER) \
    (HITS) = 0;\
    (INSERTS) = 0;\
    (COLLISIONS) = 0;\
    while (DENSITY_LIKELY((IN) < (IN_LIMIT))) {\
        shift = 0;\
        for(uint_fast8_t unroll = 0; unroll < 4; unroll++) {\
            DENSITY_UNROLL_16(\
                DENSITY_MEMCPY(&(MEMCOPY), IN, BYTE_GROUP_SIZE);\
                unit = DENSITY_ALGORITHMS_EXTRACT_64(MEMCOPY, BYTE_GROUP_SIZE);\
                hash = DENSITY_ALGORITHMS_MULTIPLY_SHIFT_64(unit, HASH_BITS);\
                value = &(DICTIONARY)->entries[hash];\
                (IN) += (BYTE_GROUP_SIZE);\
                switch (unit ^ *value) {\
                    case 0:\
                        (HITS)++;\
                        (SIGNATURE) |= ((uint64_t) DENSITY_CHAMELEON_SIGNATURE_FLAG_MAP << shift);\
                        DENSITY_MEMCPY(OUT, &hash, (HASH_BITS) >> 3);\
                        (OUT) += ((HASH_BITS) >> 3);\
                        break;\
                    default:\
                        *value = unit;\
                        uint64_t *const bitmap = &(DICTIONARY)->bitmap[hash >> 6];\
                        const uint64_t mask = ((uint64_t) 1 << (hash & 0x3f));\
                        const bool was_not_set = !(*bitmap & mask);\
                        (INSERTS) += was_not_set;\
                        (COLLISIONS) += !was_not_set;\
                        *bitmap = *bitmap | mask;\
                        DENSITY_MEMCPY(OUT, &unit, BYTE_GROUP_SIZE);\
                        (OUT) += BYTE_GROUP_SIZE;\
                        break;\
                }\
                shift ++;\
            );\
        }\
        DENSITY_CHAMELEON_PUSH_SIGNATURE(OUT, SIGNATURE, SIGNATURE_POINTER);\
        const uint_fast64_t groups_read = hits + inserts + collisions;\
        if (DENSITY_UNLIKELY(!(groups_read & (0x7ff)))) {\
            (TOTAL_INSERTS) += (INSERTS);\
            const double fill = ((double)(TOTAL_INSERTS)) / ((uint64_t) 1 << (HASH_BITS));\
            if (!(INSERTS)) {\
                if (fill > DENSITY_ALGORITHMS_NINE_TENTHS) {\
                    if ((HASH_BITS) < DENSITY_ALGORITHMS_MAX_DICTIONARY_BITS && (BYTE_GROUP_SIZE) <= 6) {\
                        DENSITY_MEMSET(&(DICTIONARY)->bitmap, 0, ((uint32_t) 1 << (DENSITY_ADD(HASH_BITS,8))) >> 3);\
                        DENSITY_ALGORITHMS_PRINT_SWITCH(HASH_BITS, BYTE_GROUP_SIZE, DENSITY_ADD(HASH_BITS,8), DENSITY_ADD(BYTE_GROUP_SIZE,2));\
                        if(DICTIONARY_CLEAR) {\
                            DENSITY_CHAMELEON_ENCODE_GENERATE_TRANSITION_TO(IN, IN_LIMIT, OUT, OUT_LIMIT, HASH_BITS, BYTE_GROUP_SIZE, DENSITY_ADD(HASH_BITS,8), DENSITY_ADD(BYTE_GROUP_SIZE,2), DICTIONARY, DENSITY_ALGORITHMS_TRANSITION_ROUNDS(DENSITY_ADD(HASH_BITS,8)), MEMCOPY, TRANSITION_COUNTER, TOTAL_INSERTS, SIGNATURE, SIGNATURE_POINTER, );\
                            goto DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(kernel_,DENSITY_ADD(HASH_BITS,8)),DENSITY_EVAL_CONCAT(_,DENSITY_ADD(BYTE_GROUP_SIZE,2)));\
                        } else {\
                            DENSITY_CHAMELEON_ENCODE_GENERATE_TRANSITION_TO(IN, IN_LIMIT, OUT, OUT_LIMIT, HASH_BITS, BYTE_GROUP_SIZE, DENSITY_ADD(HASH_BITS,8), DENSITY_ADD(BYTE_GROUP_SIZE,2), DICTIONARY, DENSITY_ALGORITHMS_TRANSITION_ROUNDS(DENSITY_ADD(HASH_BITS,8)), MEMCOPY, TRANSITION_COUNTER, TOTAL_INSERTS, SIGNATURE, SIGNATURE_POINTER, DENSITY_CHAMELEON_ENCODE_CLEAR_DICTIONARY(DENSITY_ADD(HASH_BITS,8), DENSITY_ALGORITHMS_TRANSITION_ROUNDS(DENSITY_ADD(HASH_BITS,8)), DICTIONARY, TRANSITION_COUNTER));\
                            (DICTIONARY_CLEAR) = true;\
                            goto DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(kernel_,DENSITY_ADD(HASH_BITS,8)),DENSITY_EVAL_CONCAT(_,DENSITY_ADD(BYTE_GROUP_SIZE,2)));\
                        }\
                    } else if ((COLLISIONS) > (HITS)) {\
                        DENSITY_MEMSET(&(DICTIONARY)->bitmap, 0, ((uint32_t) 1 << 8) >> 3);\
                        DENSITY_ALGORITHMS_PRINT_SWITCH(HASH_BITS, BYTE_GROUP_SIZE, 8, 2);\
                        DENSITY_CHAMELEON_ENCODE_GENERATE_TRANSITION_TO(IN, IN_LIMIT, OUT, OUT_LIMIT, HASH_BITS, BYTE_GROUP_SIZE, 8, 2, DICTIONARY, DENSITY_ALGORITHMS_TRANSITION_ROUNDS(8), MEMCOPY, TRANSITION_COUNTER, TOTAL_INSERTS, SIGNATURE, SIGNATURE_POINTER, );\
                        goto kernel_8_2;\
                    }\
                } else if (fill < DENSITY_ALGORITHMS_ONE_THIRD) {\
                    if ((BYTE_GROUP_SIZE) <= 4) {\
                        DENSITY_MEMSET(&(DICTIONARY)->bitmap, 0, ((uint32_t) 1 << (HASH_BITS)) >> 3);\
                        DENSITY_ALGORITHMS_PRINT_SWITCH(HASH_BITS, BYTE_GROUP_SIZE, HASH_BITS, DENSITY_ADD(BYTE_GROUP_SIZE,4));\
                        DENSITY_CHAMELEON_ENCODE_GENERATE_TRANSITION_TO(IN, IN_LIMIT, OUT, OUT_LIMIT, HASH_BITS, BYTE_GROUP_SIZE, HASH_BITS, DENSITY_ADD(BYTE_GROUP_SIZE,4), DICTIONARY, DENSITY_ALGORITHMS_TRANSITION_ROUNDS(HASH_BITS), MEMCOPY, TRANSITION_COUNTER, TOTAL_INSERTS, SIGNATURE, SIGNATURE_POINTER, );\
                        goto DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(kernel_,HASH_BITS),DENSITY_EVAL_CONCAT(_,DENSITY_ADD(BYTE_GROUP_SIZE,4)));\
                    } else if ((BYTE_GROUP_SIZE) <= 6) {\
                        DENSITY_MEMSET(&(DICTIONARY)->bitmap, 0, ((uint32_t) 1 << (HASH_BITS)) >> 3);\
                        DENSITY_ALGORITHMS_PRINT_SWITCH(HASH_BITS, BYTE_GROUP_SIZE, HASH_BITS, DENSITY_ADD(BYTE_GROUP_SIZE,2));\
                        DENSITY_CHAMELEON_ENCODE_GENERATE_TRANSITION_TO(IN, IN_LIMIT, OUT, OUT_LIMIT, HASH_BITS, BYTE_GROUP_SIZE, HASH_BITS, DENSITY_ADD(BYTE_GROUP_SIZE,2), DICTIONARY, DENSITY_ALGORITHMS_TRANSITION_ROUNDS(HASH_BITS), MEMCOPY, TRANSITION_COUNTER, TOTAL_INSERTS, SIGNATURE, SIGNATURE_POINTER, );\
                        goto DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(kernel_,HASH_BITS),DENSITY_EVAL_CONCAT(_,DENSITY_ADD(BYTE_GROUP_SIZE,2)));\
                    }\
                } else if (fill < DENSITY_ALGORITHMS_TWO_THIRDS) {\
                    if ((BYTE_GROUP_SIZE) <= 6) {\
                        DENSITY_MEMSET(&(DICTIONARY)->bitmap, 0, ((uint32_t) 1 << (HASH_BITS)) >> 3);\
                        DENSITY_ALGORITHMS_PRINT_SWITCH(HASH_BITS, BYTE_GROUP_SIZE, HASH_BITS, DENSITY_ADD(BYTE_GROUP_SIZE,2));\
                        DENSITY_CHAMELEON_ENCODE_GENERATE_TRANSITION_TO(IN, IN_LIMIT, OUT, OUT_LIMIT, HASH_BITS, BYTE_GROUP_SIZE, HASH_BITS, DENSITY_ADD(BYTE_GROUP_SIZE,2), DICTIONARY, DENSITY_ALGORITHMS_TRANSITION_ROUNDS(HASH_BITS), MEMCOPY, TRANSITION_COUNTER, TOTAL_INSERTS, SIGNATURE, SIGNATURE_POINTER, );\
                        goto DENSITY_EVAL_CONCAT(DENSITY_EVAL_CONCAT(kernel_,HASH_BITS),DENSITY_EVAL_CONCAT(_,DENSITY_ADD(BYTE_GROUP_SIZE,2)));\
                    }\
                }\
            }\
            (HITS) = 0;\
            (INSERTS) = 0;\
            (COLLISIONS) = 0;\
        }\
    }

DENSITY_WINDOWS_EXPORT density_algorithm_exit_status density_chameleon_encode(density_algorithm_state *DENSITY_RESTRICT_DECLARE, const uint8_t **DENSITY_RESTRICT_DECLARE, uint_fast64_t, uint8_t **DENSITY_RESTRICT_DECLARE, uint_fast64_t);

#endif
