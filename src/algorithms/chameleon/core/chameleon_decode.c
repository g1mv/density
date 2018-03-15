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
 * 23/06/15 22:11
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

#include "chameleon_decode.h"

DENSITY_WINDOWS_EXPORT DENSITY_FORCE_INLINE density_algorithm_exit_status density_chameleon_decode(density_algorithm_state *const DENSITY_RESTRICT state, const uint8_t **DENSITY_RESTRICT in, const uint_fast64_t in_size, uint8_t **DENSITY_RESTRICT out, const uint_fast64_t out_size) {
    if (out_size < DENSITY_CHAMELEON_DECODE_OUT_SAFE_DISTANCE(1, 2)) {
        return DENSITY_ALGORITHMS_EXIT_STATUS_OUTPUT_STALL;
    }

    uint_fast64_t hits = 0;
    uint_fast64_t inserts = 0;
    uint_fast64_t total_inserts = 0;
    uint_fast64_t collisions = 0;

    uint64_t memcopy_64;
    uint_fast32_t transition_counter = 0;
    uint_fast32_t samples_counter = 0;
    bool cleared = false;

    density_chameleon_signature signature;
    density_chameleon_dictionary *const dictionary = (density_chameleon_dictionary *) state->dictionary;

    const uint8_t *const in_array = *in;
    uint_fast64_t in_position = 0;
    uint_fast64_t in_limit;
    uint8_t *const out_array = *out;
    uint_fast64_t out_position = 0;
    uint_fast64_t out_limit;

    uint_fast8_t shift;
    uint64_t unit;
    uint64_t hash = 0;
    uint_fast32_t stability;

    // Study kernels
    study_kernel_8_2:
DENSITY_CHAMELEON_DECODE_GENERATE_STUDY_KERNEL(8, 2);

    study_kernel_8_4:
DENSITY_CHAMELEON_DECODE_GENERATE_STUDY_KERNEL(8, 4);

    study_kernel_8_6:
DENSITY_CHAMELEON_DECODE_GENERATE_STUDY_KERNEL(8, 6);

    study_kernel_8_8:
DENSITY_CHAMELEON_DECODE_GENERATE_STUDY_KERNEL(8, 8);

    study_kernel_16_4:
DENSITY_CHAMELEON_DECODE_GENERATE_STUDY_KERNEL(16, 4);

    study_kernel_16_6:
DENSITY_CHAMELEON_DECODE_GENERATE_STUDY_KERNEL(16, 6);

    study_kernel_16_8:
DENSITY_CHAMELEON_DECODE_GENERATE_STUDY_KERNEL(16, 8);

    // Fast kernels
    fast_kernel_8_2:
DENSITY_CHAMELEON_DECODE_GENERATE_FAST_KERNEL(8, 2);

    fast_kernel_8_4:
DENSITY_CHAMELEON_DECODE_GENERATE_FAST_KERNEL(8, 4);

    fast_kernel_8_6:
DENSITY_CHAMELEON_DECODE_GENERATE_FAST_KERNEL(8, 6);

    fast_kernel_8_8:
DENSITY_CHAMELEON_DECODE_GENERATE_FAST_KERNEL(8, 8);

    fast_kernel_16_4:
DENSITY_CHAMELEON_DECODE_GENERATE_FAST_KERNEL(16, 4);

    fast_kernel_16_6:
DENSITY_CHAMELEON_DECODE_GENERATE_FAST_KERNEL(16, 6);

    fast_kernel_16_8:
DENSITY_CHAMELEON_DECODE_GENERATE_FAST_KERNEL(16, 8);

    // Completion kernels
    completion_kernel_8_2:
DENSITY_CHAMELEON_DECODE_GENERATE_COMPLETION_KERNEL(8, 2);

    completion_kernel_8_4:
DENSITY_CHAMELEON_DECODE_GENERATE_COMPLETION_KERNEL(8, 4);

    completion_kernel_8_6:
DENSITY_CHAMELEON_DECODE_GENERATE_COMPLETION_KERNEL(8, 6);

    completion_kernel_8_8:
DENSITY_CHAMELEON_DECODE_GENERATE_COMPLETION_KERNEL(8, 8);

    completion_kernel_16_4:
DENSITY_CHAMELEON_DECODE_GENERATE_COMPLETION_KERNEL(16, 4);

    completion_kernel_16_6:
DENSITY_CHAMELEON_DECODE_GENERATE_COMPLETION_KERNEL(16, 6);

    completion_kernel_16_8:
DENSITY_CHAMELEON_DECODE_GENERATE_COMPLETION_KERNEL(16, 8);

    study_kernel_8_10:
    study_kernel_8_12:
    study_kernel_16_10:
    study_kernel_16_12:
    study_kernel_24_6:
    study_kernel_24_8:
    study_kernel_24_10:

    finish:;
    const uint_fast64_t remaining = in_size - in_position;
    if (out_position + remaining > out_size) {
        goto output_stall;
    }
    DENSITY_MEMCPY(&out_array[out_position], &in_array[in_position], remaining);
    *in += in_size;
    *out += out_position + remaining;
    return DENSITY_ALGORITHMS_EXIT_STATUS_FINISHED;

    output_stall:
    *in += in_position;
    *out += out_position;
    return DENSITY_ALGORITHMS_EXIT_STATUS_OUTPUT_STALL;
}
