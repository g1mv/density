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
 * 23/06/15 22:02
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

#include "chameleon_encode.h"

DENSITY_WINDOWS_EXPORT DENSITY_FORCE_INLINE density_algorithm_exit_status density_chameleon_encode(density_algorithm_state *const DENSITY_RESTRICT state, const uint8_t **DENSITY_RESTRICT in, const uint_fast64_t in_size, uint8_t **DENSITY_RESTRICT out, const uint_fast64_t out_size) {
    if (out_size < DENSITY_CHAMELEON_MAXIMUM_COMPRESSED_UNIT_SIZE)
        return DENSITY_ALGORITHMS_EXIT_STATUS_OUTPUT_STALL;

    uint_fast64_t hits = 0;
    uint_fast64_t inserts = 0;
    uint_fast64_t total_inserts = 0;
    uint_fast64_t collisions = 0;

    uint64_t memcopy_64;
//    const uint8_t *limit_in = *in + in_size - 8;
    uint_fast64_t counter = 0;
    const uint_fast64_t limit = in_size - sizeof(uint64_t);
    uint_fast32_t transition_counter = 0;
    bool cleared_16bits = false;

    density_chameleon_signature signature;
    density_chameleon_signature* signature_pointer;

    uint8_t *out_limit = *out + out_size - DENSITY_CHAMELEON_MAXIMUM_COMPRESSED_UNIT_SIZE;

    DENSITY_CHAMELEON_ENCODE_PREPARE_SIGNATURE(*out, signature, signature_pointer);

    uint_fast8_t shift;
    uint64_t unit;
    uint64_t hash;
    uint64_t* value;
    uint_fast32_t stability;

    study_kernel_8_2:
DENSITY_CHAMELEON_ENCODE_GENERATE_STUDY_KERNEL(counter, limit, *out, out_limit, 8, 2, (density_chameleon_dictionary *) state->dictionary, cleared_16bits, memcopy_64, hits, inserts, total_inserts, collisions, transition_counter, signature, signature_pointer);

    study_kernel_8_4:
DENSITY_CHAMELEON_ENCODE_GENERATE_STUDY_KERNEL(counter, limit, *out, out_limit, 8, 4, (density_chameleon_dictionary *) state->dictionary, cleared_16bits, memcopy_64, hits, inserts, total_inserts, collisions, transition_counter, signature, signature_pointer);

    study_kernel_8_6:
DENSITY_CHAMELEON_ENCODE_GENERATE_STUDY_KERNEL(counter, limit, *out, out_limit, 8, 6, (density_chameleon_dictionary *) state->dictionary, cleared_16bits, memcopy_64, hits, inserts, total_inserts, collisions, transition_counter, signature, signature_pointer);

    study_kernel_8_8:
DENSITY_CHAMELEON_ENCODE_GENERATE_STUDY_KERNEL(counter, limit, *out, out_limit, 8, 8, (density_chameleon_dictionary *) state->dictionary, cleared_16bits, memcopy_64, hits, inserts, total_inserts, collisions, transition_counter, signature, signature_pointer);

    study_kernel_16_4:
DENSITY_CHAMELEON_ENCODE_GENERATE_STUDY_KERNEL(counter, limit, *out, out_limit, 16, 4, (density_chameleon_dictionary *) state->dictionary, cleared_16bits, memcopy_64, hits, inserts, total_inserts, collisions, transition_counter, signature, signature_pointer);

    study_kernel_16_6:
DENSITY_CHAMELEON_ENCODE_GENERATE_STUDY_KERNEL(counter, limit, *out, out_limit, 16, 6, (density_chameleon_dictionary *) state->dictionary, cleared_16bits, memcopy_64, hits, inserts, total_inserts, collisions, transition_counter, signature, signature_pointer);

    study_kernel_16_8:
DENSITY_CHAMELEON_ENCODE_GENERATE_STUDY_KERNEL(counter, limit, *out, out_limit, 16, 8, (density_chameleon_dictionary *) state->dictionary, cleared_16bits, memcopy_64, hits, inserts, total_inserts, collisions, transition_counter, signature, signature_pointer);

    study_kernel_8_10:
    study_kernel_8_12:
    study_kernel_16_10:
    study_kernel_16_12:
    study_kernel_24_6:
    study_kernel_24_8:
    study_kernel_24_10:
    goto finished;

    fast_kernel_8_2:
DENSITY_CHAMELEON_ENCODE_GENERATE_FAST_KERNEL(counter, limit, *out, out_limit, 8, 2, (density_chameleon_dictionary *) state->dictionary, cleared_16bits, memcopy_64, hits, inserts, total_inserts, collisions, transition_counter, signature, signature_pointer);

    fast_kernel_8_4:
DENSITY_CHAMELEON_ENCODE_GENERATE_FAST_KERNEL(counter, limit, *out, out_limit, 8, 4, (density_chameleon_dictionary *) state->dictionary, cleared_16bits, memcopy_64, hits, inserts, total_inserts, collisions, transition_counter, signature, signature_pointer);

    fast_kernel_8_6:
DENSITY_CHAMELEON_ENCODE_GENERATE_FAST_KERNEL(counter, limit, *out, out_limit, 8, 6, (density_chameleon_dictionary *) state->dictionary, cleared_16bits, memcopy_64, hits, inserts, total_inserts, collisions, transition_counter, signature, signature_pointer);

    fast_kernel_8_8:
DENSITY_CHAMELEON_ENCODE_GENERATE_FAST_KERNEL(counter, limit, *out, out_limit, 8, 8, (density_chameleon_dictionary *) state->dictionary, cleared_16bits, memcopy_64, hits, inserts, total_inserts, collisions, transition_counter, signature, signature_pointer);

    fast_kernel_16_4:
DENSITY_CHAMELEON_ENCODE_GENERATE_FAST_KERNEL(counter, limit, *out, out_limit, 16, 4, (density_chameleon_dictionary *) state->dictionary, cleared_16bits, memcopy_64, hits, inserts, total_inserts, collisions, transition_counter, signature, signature_pointer);

    fast_kernel_16_6:
DENSITY_CHAMELEON_ENCODE_GENERATE_FAST_KERNEL(counter, limit, *out, out_limit, 16, 6, (density_chameleon_dictionary *) state->dictionary, cleared_16bits, memcopy_64, hits, inserts, total_inserts, collisions, transition_counter, signature, signature_pointer);

    fast_kernel_16_8:
DENSITY_CHAMELEON_ENCODE_GENERATE_FAST_KERNEL(counter, limit, *out, out_limit, 16, 8, (density_chameleon_dictionary *) state->dictionary, cleared_16bits, memcopy_64, hits, inserts, total_inserts, collisions, transition_counter, signature, signature_pointer);


finished:
//    printf("-> %llu bytes\n", (unsigned long long) (total_bits >> 3));

    return DENSITY_ALGORITHMS_EXIT_STATUS_FINISHED;//todo

//    if (*out > out_limit)
//        return DENSITY_ALGORITHMS_EXIT_STATUS_OUTPUT_STALL;
//
//    uint_fast64_t remaining;
//
//    switch (in_size & 0xff) {
//        case 0:
//        case 1:
//        case 2:
//        case 3:
//            density_chameleon_encode_prepare_signature(out, &signature_pointer, &signature);
//            signature = ((uint64_t) DENSITY_CHAMELEON_SIGNATURE_FLAG_CHUNK);    // End marker
//#ifdef DENSITY_LITTLE_ENDIAN
//            DENSITY_MEMCPY(signature_pointer, &signature, sizeof(density_chameleon_signature));
//#elif defined(DENSITY_BIG_ENDIAN)
//        const density_chameleon_signature endian_signature = DENSITY_LITTLE_ENDIAN_64(signature);
//            DENSITY_MEMCPY(signature_pointer, &endian_signature, sizeof(density_chameleon_signature));
//#else
//#error
//#endif
//            goto process_remaining_bytes;
//        default:
//            break;
//    }
//
//    const uint_fast64_t limit_4 = (in_size & 0xff) >> 2;
//    density_chameleon_encode_prepare_signature(out, &signature_pointer, &signature);
//    for (uint_fast8_t shift = 0; shift != limit_4; shift++)
//        density_chameleon_encode_4(in, out, shift, &signature, (density_chameleon_dictionary *const) state->dictionary, &unit);
//
//    signature |= ((uint64_t) DENSITY_CHAMELEON_SIGNATURE_FLAG_CHUNK << limit_4);    // End marker
//#ifdef DENSITY_LITTLE_ENDIAN
//    DENSITY_MEMCPY(signature_pointer, &signature, sizeof(density_chameleon_signature));
//#elif defined(DENSITY_BIG_ENDIAN)
//    const density_chameleon_signature endian_signature = DENSITY_LITTLE_ENDIAN_64(signature);
//    DENSITY_MEMCPY(signature_pointer, &endian_signature, sizeof(density_chameleon_signature));
//#else
//#error
//#endif
//
//    process_remaining_bytes:
//    remaining = in_size & 0x3;
//    if (remaining) {
//        DENSITY_ALGORITHM_COPY(remaining);
//    }
//
//    return DENSITY_ALGORITHMS_EXIT_STATUS_FINISHED;
}
