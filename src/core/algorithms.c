/*
 * Centaurean Density
 *
 * Copyright (c) 2015, Guillaume Voirin
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
 * 14/10/15 02:06
 */

#include "algorithms.h"

DENSITY_WINDOWS_EXPORT DENSITY_FORCE_INLINE void density_algorithms_prepare_state(density_algorithm_state *const restrict state, void *const restrict dictionary) {
    state->dictionary = dictionary;
    state->copy_penalty = 0;
    state->copy_penalty_start = 1;
    state->counter = 0;
    state->user_defined_interrupt = 0;
    state->return_from_interrupt = false;
}

DENSITY_WINDOWS_EXPORT DENSITY_FORCE_INLINE void density_algorithm_copy(density_algorithm_state *const state, const uint8_t **restrict in, uint8_t **restrict out, const uint_fast16_t work_block_size) {
    DENSITY_MEMCPY(*out, *in, work_block_size);
    *in += work_block_size;
    *out += work_block_size;
    if (!(--state->copy_penalty))
        state->copy_penalty_start++;
}