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

DENSITY_WINDOWS_EXPORT const uint_fast64_t density_buffer_compress_safe_size(const uint_fast64_t input_size) {
    uint_fast64_t longest_output_size = 0;

    // Chameleon longest output
    uint_fast64_t chameleon_longest_output_size = 0;
    chameleon_longest_output_size += sizeof(density_header);
    chameleon_longest_output_size += sizeof(density_chameleon_signature) * (1 + (input_size >> (5 + 3)));   // Signature space (1 bit <=> 4 bytes)
    chameleon_longest_output_size += sizeof(density_chameleon_signature);                                   // Eventual supplementary signature for end marker
    chameleon_longest_output_size += input_size;                                                            // Everything encoded as plain data
    chameleon_longest_output_size += sizeof(density_footer);                                                // In case of integrity checks
    longest_output_size = chameleon_longest_output_size;

    // Cheetah longest output
    uint_fast64_t cheetah_longest_output_size = 0;
    cheetah_longest_output_size += sizeof(density_header);
    cheetah_longest_output_size += sizeof(density_cheetah_signature) * (1 + (input_size >> (4 + 3)));       // Signature space (2 bits <=> 4 bytes)
    cheetah_longest_output_size += sizeof(density_cheetah_signature);                                       // Eventual supplementary signature for end marker
    cheetah_longest_output_size += input_size;                                                              // Everything encoded as plain data
    cheetah_longest_output_size += sizeof(density_footer);                                                  // In case of integrity checks
    if (cheetah_longest_output_size > longest_output_size)
        longest_output_size = cheetah_longest_output_size;

    // Lion longest output
    uint_fast64_t lion_longest_output_size = 0;
    lion_longest_output_size += sizeof(density_header);
    lion_longest_output_size += sizeof(density_lion_signature) * (1 + ((input_size * 7) >> (5 + 3)));       // Signature space (7 bits <=> 4 bytes), although this size is technically impossible
    lion_longest_output_size += sizeof(density_lion_signature);                                             // Eventual supplementary signature for end marker
    lion_longest_output_size += input_size;                                                                 // Everything encoded as plain data
    lion_longest_output_size += sizeof(density_footer);                                                     // In case of integrity checks
    if (lion_longest_output_size > longest_output_size)
        longest_output_size = lion_longest_output_size;

    return longest_output_size;
}

DENSITY_WINDOWS_EXPORT const uint_fast64_t density_buffer_decompress_safe_size(const uint_fast64_t expected_output_size) {
    uint_fast64_t slack = DENSITY_CHAMELEON_DECOMPRESSED_UNIT_SIZE;
    if (DENSITY_CHEETAH_DECOMPRESSED_UNIT_SIZE > slack)
        slack = DENSITY_CHEETAH_DECOMPRESSED_UNIT_SIZE;
    if (DENSITY_LION_MAXIMUM_DECOMPRESSED_UNIT_SIZE > slack)
        slack = DENSITY_LION_MAXIMUM_DECOMPRESSED_UNIT_SIZE;

    return expected_output_size + slack;
}

DENSITY_FORCE_INLINE const DENSITY_BUFFER_STATE density_buffer_convert_algorithm_exit_status(const density_algorithms_exit_status status) {
    switch (status) {
        case DENSITY_ALGORITHMS_EXIT_STATUS_FINISHED:
            return DENSITY_BUFFER_STATE_OK;
        case DENSITY_ALGORITHMS_EXIT_STATUS_ERROR_DURING_PROCESSING:
            return DENSITY_BUFFER_STATE_ERROR_DURING_PROCESSING;
        case DENSITY_ALGORITHMS_EXIT_STATUS_INPUT_STALL:
            return DENSITY_BUFFER_STATE_ERROR_INPUT_BUFFER_TOO_SMALL;
        case DENSITY_ALGORITHMS_EXIT_STATUS_OUTPUT_STALL:
            return DENSITY_BUFFER_STATE_ERROR_OUTPUT_BUFFER_TOO_SMALL;
    }
}

DENSITY_FORCE_INLINE const density_buffer_processing_result density_buffer_make_result(const DENSITY_BUFFER_STATE state, const uint_fast64_t read, const uint_fast64_t written) {
    density_buffer_processing_result result;
    result.state = state;
    result.bytesRead = read;
    result.bytesWritten = written;
    return result;
}

DENSITY_WINDOWS_EXPORT DENSITY_FORCE_INLINE const density_buffer_processing_result density_buffer_compress(const uint8_t *restrict input_buffer, const uint_fast64_t input_size, uint8_t *restrict output_buffer, const uint_fast64_t output_size, const DENSITY_COMPRESSION_MODE compression_mode, const DENSITY_BLOCK_TYPE block_type) {
    // Variables setup
    density_algorithms_exit_status status;
    const uint8_t *in = input_buffer;
    uint8_t *out = output_buffer;

    // Header
    density_header_write_unrestricted(&out, compression_mode, block_type);

    // Compression
    switch (compression_mode) {
        case DENSITY_COMPRESSION_MODE_COPY:
            DENSITY_MEMCPY(out, in, input_size);
            in += input_size;
            out += input_size;
            break;
        case DENSITY_COMPRESSION_MODE_CHAMELEON_ALGORITHM: {
            density_chameleon_dictionary dictionary;
            density_chameleon_dictionary_reset(&dictionary);
            if ((status = density_chameleon_encode(&in, input_size, &out, output_size, &dictionary, true)))
                return density_buffer_make_result(density_buffer_convert_algorithm_exit_status(status), in - input_buffer, out - output_buffer);
            break;
        }
        case DENSITY_COMPRESSION_MODE_CHEETAH_ALGORITHM:
            if ((status = density_cheetah_encode(&in, input_size, &out, output_size, true)))
                return density_buffer_make_result(density_buffer_convert_algorithm_exit_status(status), in - input_buffer, out - output_buffer);
            break;
        case DENSITY_COMPRESSION_MODE_LION_ALGORITHM:
            if ((status = density_lion_encode(&in, input_size, &out, output_size, true)))
                return density_buffer_make_result(density_buffer_convert_algorithm_exit_status(status), in - input_buffer, out - output_buffer);
            break;
        default:
            break;
    }

    // Footer
    if (block_type == DENSITY_BLOCK_TYPE_WITH_HASHSUM_INTEGRITY_CHECK) {
        uint64_t hash1, hash2;
        hash1 = DENSITY_SPOOKYHASH_SEED_1;
        hash2 = DENSITY_SPOOKYHASH_SEED_2;
        spookyhash_128(input_buffer, input_size, &hash1, &hash2);
        density_footer_write_unrestricted(&out, hash1, hash2);
    }

    // Result
    return density_buffer_make_result(DENSITY_BUFFER_STATE_OK, in - input_buffer, out - output_buffer);
}

DENSITY_WINDOWS_EXPORT DENSITY_FORCE_INLINE const density_buffer_processing_result density_buffer_decompress(const uint8_t *restrict input_buffer, const uint_fast64_t input_size, uint8_t *restrict output_buffer, const uint_fast64_t output_size) {
    if (input_size < sizeof(density_header) + sizeof(uint64_t))
        density_buffer_make_result(DENSITY_BUFFER_STATE_ERROR_INPUT_BUFFER_TOO_SMALL, 0, 0);

    // Variables setup
    density_algorithms_exit_status status;
    const uint8_t *in = input_buffer;
    uint8_t *out = output_buffer;

    // Header
    density_header main_header;
    density_header_read_unrestricted(&in, &main_header);
    const bool integrity_checks = (main_header.blockType == DENSITY_BLOCK_TYPE_WITH_HASHSUM_INTEGRITY_CHECK);
    uint_fast64_t remaining = input_size - (in - input_buffer);
    if (integrity_checks)
        remaining -= sizeof(density_footer);

    // Decompression
    switch (main_header.compressionMode) {
        case DENSITY_COMPRESSION_MODE_COPY:
            DENSITY_MEMCPY(out, in, remaining);
            in += remaining;
            out += remaining;
            break;
        case DENSITY_COMPRESSION_MODE_CHAMELEON_ALGORITHM: {
            density_chameleon_dictionary dictionary;
            density_chameleon_dictionary_reset(&dictionary);
            if ((status = density_chameleon_decode(&in, remaining, &out, output_size, &dictionary)))
                return density_buffer_make_result(density_buffer_convert_algorithm_exit_status(status), in - input_buffer, out - output_buffer);
            break;
        }
        case DENSITY_COMPRESSION_MODE_CHEETAH_ALGORITHM:
            if ((status = density_cheetah_decode(&in, remaining, &out, output_size)))
                return density_buffer_make_result(density_buffer_convert_algorithm_exit_status(status), in - input_buffer, out - output_buffer);
            break;
        case DENSITY_COMPRESSION_MODE_LION_ALGORITHM:
            if ((status = density_lion_decode(&in, remaining, &out, output_size)))
                return density_buffer_make_result(density_buffer_convert_algorithm_exit_status(status), in - input_buffer, out - output_buffer);
            break;
        default:
            break;
    }

    // Footer
    if (integrity_checks) {
        uint64_t hash1, hash2;
        hash1 = DENSITY_SPOOKYHASH_SEED_1;
        hash2 = DENSITY_SPOOKYHASH_SEED_2;
        spookyhash_128(output_buffer, out - output_buffer, &hash1, &hash2);
        density_footer footer;
        density_footer_read_unrestricted(&in, &footer);
        if (footer.hashsum1 != hash1 || footer.hashsum2 != hash2)
            density_buffer_make_result(DENSITY_BUFFER_STATE_ERROR_INTEGRITY_CHECK_FAIL, in - input_buffer, out - output_buffer);
    }

    // Result
    return density_buffer_make_result(DENSITY_BUFFER_STATE_OK, in - input_buffer, out - output_buffer);
}
