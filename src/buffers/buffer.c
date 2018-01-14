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
 * 3/02/15 19:53
 */

#include "buffer.h"

DENSITY_WINDOWS_EXPORT const uint_fast64_t density_compress_safe_size(const uint_fast64_t input_size) {
    const uint_fast64_t slack = DENSITY_MAX_3(DENSITY_CHAMELEON_MAXIMUM_COMPRESSED_UNIT_SIZE, DENSITY_CHEETAH_MAXIMUM_COMPRESSED_UNIT_SIZE, DENSITY_LION_MAXIMUM_COMPRESSED_UNIT_SIZE);
    uint_fast64_t longest_output_size = 0;

    // Chameleon longest output
    uint_fast64_t chameleon_longest_output_size = 0;
    chameleon_longest_output_size += sizeof(density_header);
    chameleon_longest_output_size += sizeof(density_chameleon_signature) * (1 + (input_size >> (5 + 3)));   // Signature space (1 bit <=> 4 bytes)
    chameleon_longest_output_size += sizeof(density_chameleon_signature);                                   // Eventual supplementary signature for end marker
    chameleon_longest_output_size += input_size;                                                            // Everything encoded as plain data
    longest_output_size = chameleon_longest_output_size;

    // Cheetah longest output
    uint_fast64_t cheetah_longest_output_size = 0;
    cheetah_longest_output_size += sizeof(density_header);
    cheetah_longest_output_size += sizeof(density_cheetah_signature) * (1 + (input_size >> (4 + 3)));       // Signature space (2 bits <=> 4 bytes)
    cheetah_longest_output_size += sizeof(density_cheetah_signature);                                       // Eventual supplementary signature for end marker
    cheetah_longest_output_size += input_size;                                                              // Everything encoded as plain data
    if (cheetah_longest_output_size > longest_output_size)
        longest_output_size = cheetah_longest_output_size;

    // Lion longest output
    uint_fast64_t lion_longest_output_size = 0;
    lion_longest_output_size += sizeof(density_header);
    lion_longest_output_size += sizeof(density_lion_signature) * (1 + ((input_size * 7) >> (5 + 3)));       // Signature space (7 bits <=> 4 bytes), although this size is technically impossible
    lion_longest_output_size += sizeof(density_lion_signature);                                             // Eventual supplementary signature for end marker
    lion_longest_output_size += input_size;                                                                 // Everything encoded as plain data
    if (lion_longest_output_size > longest_output_size)
        longest_output_size = lion_longest_output_size;

    return DENSITY_MAX_2(longest_output_size, slack);
}

DENSITY_WINDOWS_EXPORT const uint_fast64_t density_decompress_safe_size(const uint_fast64_t expected_output_size) {
    const uint_fast64_t slack = DENSITY_MAX_3(DENSITY_CHAMELEON_DECOMPRESSED_UNIT_SIZE, DENSITY_CHEETAH_DECOMPRESSED_UNIT_SIZE, DENSITY_LION_MAXIMUM_DECOMPRESSED_UNIT_SIZE);

    return expected_output_size + slack;
}

DENSITY_FORCE_INLINE const DENSITY_STATE density_convert_algorithm_exit_status(const density_algorithm_exit_status status) {
    switch (status) {
        case DENSITY_ALGORITHMS_EXIT_STATUS_FINISHED:
            return DENSITY_STATE_OK;
        case DENSITY_ALGORITHMS_EXIT_STATUS_ERROR_DURING_PROCESSING:
            return DENSITY_STATE_ERROR_DURING_PROCESSING;
        case DENSITY_ALGORITHMS_EXIT_STATUS_INPUT_STALL:
            return DENSITY_STATE_ERROR_INPUT_BUFFER_TOO_SMALL;
        case DENSITY_ALGORITHMS_EXIT_STATUS_OUTPUT_STALL:
            return DENSITY_STATE_ERROR_OUTPUT_BUFFER_TOO_SMALL;
    }
}

DENSITY_FORCE_INLINE const density_processing_result density_make_result(const DENSITY_STATE state, const uint_fast64_t read, const uint_fast64_t written, density_dictionary *const dictionary) {
    density_processing_result result;
    result.state = state;
    result.bytesRead = read;
    result.bytesWritten = written;
    result.dictionary = dictionary;
    return result;
}

DENSITY_WINDOWS_EXPORT const density_processing_result density_compress(const uint8_t *input_buffer, const uint_fast64_t input_size, uint8_t *output_buffer, const uint_fast64_t output_size, const DENSITY_ALGORITHM algorithm) {
    density_dictionary dictionary_container;
    dictionary_container.algorithm = algorithm;

    switch(algorithm) {
        case DENSITY_ALGORITHM_CHAMELEON:
            dictionary_container.size = sizeof(density_chameleon_dictionary);
            density_chameleon_dictionary chameleon_dictionary;
            dictionary_container.pointer = &chameleon_dictionary;
            break;
        case DENSITY_ALGORITHM_CHEETAH:
            dictionary_container.size = sizeof(density_cheetah_dictionary);
            density_cheetah_dictionary cheetah_dictionary;
            dictionary_container.pointer = &cheetah_dictionary;
            break;
        case DENSITY_ALGORITHM_LION:
            dictionary_container.size = sizeof(density_lion_dictionary);
            density_lion_dictionary lion_dictionary;
            dictionary_container.pointer = &lion_dictionary;
            break;
        default:
            return density_make_result(DENSITY_STATE_ERROR_INVALID_ALGORITHM, 0, 0, NULL);
    }

    return density_decompress_with_dictionary(input_buffer, input_size, output_buffer, output_size, &dictionary_container);
}

DENSITY_WINDOWS_EXPORT const density_processing_result density_decompress(const uint8_t *input_buffer, const uint_fast64_t input_size, uint8_t *output_buffer, const uint_fast64_t output_size, const DENSITY_ALGORITHM algorithm) {
    density_dictionary dictionary_container;
    dictionary_container.algorithm = algorithm;

    switch(algorithm) {
        case DENSITY_ALGORITHM_CHAMELEON:
            dictionary_container.size = sizeof(density_chameleon_dictionary);
            density_chameleon_dictionary chameleon_dictionary;
            dictionary_container.pointer = &chameleon_dictionary;
            break;
        case DENSITY_ALGORITHM_CHEETAH:
            dictionary_container.size = sizeof(density_cheetah_dictionary);
            density_cheetah_dictionary cheetah_dictionary;
            dictionary_container.pointer = &cheetah_dictionary;
            break;
        case DENSITY_ALGORITHM_LION:
            dictionary_container.size = sizeof(density_lion_dictionary);
            density_lion_dictionary lion_dictionary;
            dictionary_container.pointer = &lion_dictionary;
            break;
        default:
            return density_make_result(DENSITY_STATE_ERROR_INVALID_ALGORITHM, 0, 0, NULL);
    }

    return density_decompress_with_dictionary(input_buffer, input_size, output_buffer, output_size, &dictionary_container);
}

DENSITY_WINDOWS_EXPORT DENSITY_FORCE_INLINE const density_processing_result density_compress_with_dictionary(const uint8_t *restrict input_buffer, const uint_fast64_t input_size, uint8_t *restrict output_buffer, const uint_fast64_t output_size, density_dictionary *const dictionary) {
    if (output_size < sizeof(density_header))
        return density_make_result(DENSITY_STATE_ERROR_OUTPUT_BUFFER_TOO_SMALL, 0, 0, dictionary);
    if(dictionary == NULL)
        return density_make_result(DENSITY_STATE_ERROR_INVALID_DICTIONARY, 0, 0, dictionary);

    // Variables setup
    const uint8_t *in = input_buffer;
    uint8_t *out = output_buffer;
    density_algorithm_state state;

    // Header
    density_header_write(&out, dictionary->algorithm);

    // Compression
    switch (dictionary->algorithm) {
        case DENSITY_ALGORITHM_CHAMELEON: {
            density_algorithms_prepare_state(&state, dictionary->pointer);
            return density_make_result(density_convert_algorithm_exit_status(density_chameleon_encode(&state, &in, input_size, &out, output_size)), in - input_buffer, out - output_buffer, dictionary);
        }
        case DENSITY_ALGORITHM_CHEETAH: {
            density_algorithms_prepare_state(&state, dictionary->pointer);
            return density_make_result(density_convert_algorithm_exit_status(density_cheetah_encode(&state, &in, input_size, &out, output_size)), in - input_buffer, out - output_buffer, dictionary);
        }
        case DENSITY_ALGORITHM_LION: {
            density_algorithms_prepare_state(&state, dictionary->pointer);
            return density_make_result(density_convert_algorithm_exit_status(density_lion_encode(&state, &in, input_size, &out, output_size)), in - input_buffer, out - output_buffer, dictionary);
        }
        default:
            return density_make_result(DENSITY_STATE_ERROR_INVALID_DICTIONARY, 0, 0, dictionary);
    }

    // Result
    return density_make_result(DENSITY_STATE_OK, in - input_buffer, out - output_buffer, dictionary);
}

DENSITY_WINDOWS_EXPORT DENSITY_FORCE_INLINE const density_processing_result density_decompress_with_dictionary(const uint8_t *restrict input_buffer, const uint_fast64_t input_size, uint8_t *restrict output_buffer, const uint_fast64_t output_size, density_dictionary *const dictionary) {
    if (input_size < sizeof(density_header))
        return density_make_result(DENSITY_STATE_ERROR_INPUT_BUFFER_TOO_SMALL, 0, 0, dictionary);
    if(dictionary == NULL)
        return density_make_result(DENSITY_STATE_ERROR_INVALID_DICTIONARY, 0, 0, dictionary);

    // Variables setup
    const uint8_t *in = input_buffer;
    uint8_t *out = output_buffer;
    density_algorithm_state state;

    // Header
    density_header main_header;
    density_header_read(&in, &main_header);
    uint_fast64_t remaining = input_size - (in - input_buffer);

    // Check the dictionary provided was created for the proper algorithm
    if(dictionary->algorithm != main_header.algorithm)
        return density_make_result(DENSITY_STATE_ERROR_INVALID_DICTIONARY, 0, 0, dictionary);

    // Decompression
    switch (main_header.algorithm) {
        case DENSITY_ALGORITHM_CHAMELEON: {
            density_algorithms_prepare_state(&state, dictionary->pointer);
            return density_make_result(density_convert_algorithm_exit_status(density_chameleon_decode(&state, &in, remaining, &out, output_size)), in - input_buffer, out - output_buffer, dictionary);
        }
        case DENSITY_ALGORITHM_CHEETAH: {
            density_algorithms_prepare_state(&state, dictionary->pointer);
            return density_make_result(density_convert_algorithm_exit_status(density_cheetah_decode(&state, &in, remaining, &out, output_size)), in - input_buffer, out - output_buffer, dictionary);
        }
        case DENSITY_ALGORITHM_LION: {
            density_algorithms_prepare_state(&state, dictionary->pointer);
            return density_make_result(density_convert_algorithm_exit_status(density_lion_decode(&state, &in, remaining, &out, output_size)), in - input_buffer, out - output_buffer, dictionary);
        }
        default:
            return density_make_result(DENSITY_STATE_ERROR_INVALID_DICTIONARY, 0, 0, dictionary);
    }

    // Result
    return density_make_result(DENSITY_STATE_OK, in - input_buffer, out - output_buffer, dictionary);
}
