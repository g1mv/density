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
 * 10/04/18 12:00
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

#include "chameleon_dictionary.h"

void density_chameleon_dictionary_initialize(density_chameleon_dictionary *dictionary) {
    DENSITY_MEMSET(dictionary->bitmap, 0, ((uint32_t) 1 << DENSITY_ALGORITHMS_INITIAL_DICTIONARY_KEY_BITS) >> (uint8_t) 3);
    DENSITY_FAST_CLEAR_ARRAY_64(dictionary->entries, ((uint32_t) 1 << DENSITY_ALGORITHMS_INITIAL_DICTIONARY_KEY_BITS));
    dictionary->state.cleared = false;
    dictionary->state.active_mode = DENSITY_CHAMELEON_DICTIONARY_ACTIVE_MODE_STUDY;
    dictionary->state.active_hash_bytes = 1;
    dictionary->state.active_group_bytes = 2;
}

void density_chameleon_dictionary_update_state(density_chameleon_dictionary *dictionary, DENSITY_CHAMELEON_DICTIONARY_ACTIVE_MODE mode, uint8_t hash_bytes, uint8_t group_bytes) {
    dictionary->state.active_mode = mode;
    dictionary->state.active_hash_bytes = hash_bytes;
    dictionary->state.active_group_bytes = group_bytes;
}
