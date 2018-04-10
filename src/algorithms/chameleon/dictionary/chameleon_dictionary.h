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
 * 24/10/13 12:05
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

#ifndef DENSITY_ALGORITHMS_CHAMELEON_DICTIONARY_H
#define DENSITY_ALGORITHMS_CHAMELEON_DICTIONARY_H

#include "../../algorithms.h"
#include "../chameleon.h"

#include <string.h>

#pragma pack(push)
#pragma pack(4)

typedef enum {
    DENSITY_CHAMELEON_DICTIONARY_ACTIVE_MODE_STUDY = 1,
    DENSITY_CHAMELEON_DICTIONARY_ACTIVE_MODE_FAST = 2,
} DENSITY_CHAMELEON_DICTIONARY_ACTIVE_MODE;

typedef struct {
    DENSITY_CHAMELEON_DICTIONARY_ACTIVE_MODE active_mode;
    uint8_t active_hash_bytes;
    uint8_t active_group_bytes;
} density_dictionary_state;

typedef struct {
    density_dictionary_state state;
    uint64_t bitmap[(1u << DENSITY_ALGORITHMS_MAX_DICTIONARY_KEY_BITS) / 64];
    uint64_t entries[1u << DENSITY_ALGORITHMS_MAX_DICTIONARY_KEY_BITS];
} density_chameleon_dictionary;

#pragma pack(pop)

#define DENSITY_CHAMELEON_DICTIONARY_INITIAL_BRANCHING_FROM_STATE\
    switch(dictionary->state.active_mode) {\
        case DENSITY_CHAMELEON_DICTIONARY_ACTIVE_MODE_STUDY:\
            switch(dictionary->state.active_hash_bytes) {\
                case 1:\
                    switch(dictionary->state.active_group_bytes) {\
                        case 2:\
                            goto study_kernel_1_2;\
                        case 4:\
                            goto study_kernel_1_4;\
                        case 6:\
                            goto study_kernel_1_6;\
                        case 8:\
                            goto study_kernel_1_8;\
                        default:\
                            return DENSITY_ALGORITHMS_EXIT_STATUS_ERROR_DURING_PROCESSING;\
                    }\
                case 2:\
                    switch(dictionary->state.active_group_bytes) {\
                        case 4:\
                            goto study_kernel_2_4;\
                        case 6:\
                            goto study_kernel_2_6;\
                        case 8:\
                            goto study_kernel_2_8;\
                        default:\
                            return DENSITY_ALGORITHMS_EXIT_STATUS_ERROR_DURING_PROCESSING;\
                    }\
                default:\
                    return DENSITY_ALGORITHMS_EXIT_STATUS_ERROR_DURING_PROCESSING;\
            }\
        case DENSITY_CHAMELEON_DICTIONARY_ACTIVE_MODE_FAST:\
            switch(dictionary->state.active_hash_bytes) {\
                case 1:\
                    switch(dictionary->state.active_group_bytes) {\
                        case 2:\
                            goto fast_kernel_1_2;\
                        case 4:\
                            goto fast_kernel_1_4;\
                        case 6:\
                            goto fast_kernel_1_6;\
                        case 8:\
                            goto fast_kernel_1_8;\
                        default:\
                            return DENSITY_ALGORITHMS_EXIT_STATUS_ERROR_DURING_PROCESSING;\
                    }\
                case 2:\
                    switch(dictionary->state.active_group_bytes) {\
                        case 4:\
                            goto fast_kernel_2_4;\
                        case 6:\
                            goto fast_kernel_2_6;\
                        case 8:\
                            goto fast_kernel_2_8;\
                        default:\
                            return DENSITY_ALGORITHMS_EXIT_STATUS_ERROR_DURING_PROCESSING;\
                    }\
                default:\
                    return DENSITY_ALGORITHMS_EXIT_STATUS_ERROR_DURING_PROCESSING;\
            }\
        default:\
            return DENSITY_ALGORITHMS_EXIT_STATUS_ERROR_DURING_PROCESSING;\
    }

DENSITY_WINDOWS_EXPORT void density_chameleon_dictionary_initialize(density_chameleon_dictionary *);

DENSITY_WINDOWS_EXPORT void density_chameleon_dictionary_update_state(density_chameleon_dictionary *, DENSITY_CHAMELEON_DICTIONARY_ACTIVE_MODE, uint8_t, uint8_t);

#endif
