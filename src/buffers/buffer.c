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

#include "../api.h"
#include "../globals.h"
#include "../structure/header.h"
#include "buffer.h"

DENSITY_WINDOWS_EXPORT uint64_t density_compress_safe_output_size(const DENSITY_ALGORITHM algorithm, const uint64_t input_size) {
    uint_fast64_t safe_size = 0;

    switch (algorithm) {
        case DENSITY_ALGORITHM_CHAMELEON:
            safe_size += DENSITY_HEADER_BASE_SIZE + DENSITY_HEADER_ORIGINAL_SIZE_BYTES(input_size);     // Header size
            safe_size += 8 + ((input_size / 2) / 8);                                                    // Worst case, signature space with 2-byte groups
            safe_size += input_size;                                                                    // Everything encoded as plain data
            return safe_size;

        default:;
            const uint_fast64_t slack = DENSITY_MAXIMUM(DENSITY_CHEETAH_MAXIMUM_COMPRESSED_UNIT_SIZE, DENSITY_LION_MAXIMUM_COMPRESSED_UNIT_SIZE);

            // Cheetah longest output
            uint_fast64_t cheetah_longest_output_size = 0;
            cheetah_longest_output_size += DENSITY_HEADER_ORIGINAL_SIZE_BYTES(input_size);
            cheetah_longest_output_size += sizeof(density_cheetah_signature) * (1 + (input_size >> (uint8_t) (4 + 3))); // Signature space (2 bits <=> 4 bytes)
            cheetah_longest_output_size += sizeof(density_cheetah_signature);                                           // Eventual supplementary signature for end marker
            cheetah_longest_output_size += input_size;                                                                  // Everything encoded as plain data

            // Lion longest output
            uint_fast64_t lion_longest_output_size = 0;
            lion_longest_output_size += DENSITY_HEADER_ORIGINAL_SIZE_BYTES(input_size);
            lion_longest_output_size += sizeof(density_lion_signature) * (1 + ((input_size * 7) >> (uint8_t) (5 + 3))); // Signature space (7 bits <=> 4 bytes), although this size is technically impossible
            lion_longest_output_size += sizeof(density_lion_signature);                                                 // Eventual supplementary signature for end marker
            lion_longest_output_size += input_size;                                                                     // Everything encoded as plain data

            return DENSITY_MAXIMUM(cheetah_longest_output_size, lion_longest_output_size) + slack;
    }
}

DENSITY_STATE density_convert_algorithm_exit_status(const density_algorithm_exit_status status) {
    switch (status) {
        case DENSITY_ALGORITHMS_EXIT_STATUS_FINISHED:
            return DENSITY_STATE_OK;
        case DENSITY_ALGORITHMS_EXIT_STATUS_INPUT_STALL:
            return DENSITY_STATE_ERROR_INPUT_BUFFER_TOO_SMALL;
        case DENSITY_ALGORITHMS_EXIT_STATUS_OUTPUT_STALL:
            return DENSITY_STATE_ERROR_OUTPUT_BUFFER_TOO_SMALL;
        default:
            return DENSITY_STATE_ERROR_DURING_PROCESSING;
    }
}

density_processing_result density_make_result(const DENSITY_STATE state, const uint64_t read, const uint64_t written, density_context *const context) {
    density_processing_result result;
    result.state = state;
    result.bytes_read = read;
    result.bytes_written = written;
    result.context = context;
    return result;
}

density_context *density_allocate_context(density_metadata metadata, const bool is_dictionary_custom, void *(*mem_alloc)(size_t)) {
    density_context* context = mem_alloc(sizeof(density_context));
    context->metadata = metadata;
    context->dictionary.size = density_get_dictionary_size((DENSITY_ALGORITHM) context->metadata.algorithm);
    context->dictionary.is_custom = is_dictionary_custom;
    if (!context->dictionary.is_custom) {
        context->dictionary.pointer = mem_alloc(context->dictionary.size);
        if (context->metadata.algorithm == DENSITY_ALGORITHM_CHAMELEON) {
            // todo in progress, for adaptative chameleon, init only 8-bit bitmap and 8-bit dictionary sizes
            DENSITY_MEMSET(&((density_chameleon_dictionary *) context->dictionary.pointer)->bitmap, 0, ((uint32_t) 1 << DENSITY_ALGORITHMS_INITIAL_DICTIONARY_KEY_BITS) >> (uint8_t) 3);
            DENSITY_FAST_CLEAR_ARRAY_64(((density_chameleon_dictionary *) context->dictionary.pointer)->entries, ((uint32_t) 1 << DENSITY_ALGORITHMS_INITIAL_DICTIONARY_KEY_BITS));
        } else {
            DENSITY_MEMSET(context->dictionary.pointer, 0, context->dictionary.size);
        }
    }

    return context;
}

DENSITY_WINDOWS_EXPORT void density_free_context(density_context *const context, void (*mem_free)(void *)) {
    if (mem_free == NULL) {
        mem_free = free;
    }
    if (!context->dictionary.is_custom) {
        mem_free(context->dictionary.pointer);
    }
    mem_free(context);
}

DENSITY_WINDOWS_EXPORT density_processing_result density_compress_prepare_context(const DENSITY_ALGORITHM algorithm, const uint64_t original_size, const bool is_dictionary_custom, void *(*mem_alloc)(size_t)) {
    if (mem_alloc == NULL) {
        mem_alloc = malloc;
    }

    // Prepare metadata
    density_metadata metadata;
    metadata.version_major = density_version_major();
    metadata.version_minor = density_version_minor();
    metadata.version_revision = density_version_revision();
    metadata.algorithm = algorithm;
    metadata.original_size = original_size;

    return density_make_result(DENSITY_STATE_OK, 0, 0, density_allocate_context(metadata, is_dictionary_custom, mem_alloc));
}

DENSITY_WINDOWS_EXPORT density_processing_result density_compress_with_context(const uint8_t *input_buffer, const uint64_t input_size, uint8_t *output_buffer, const uint64_t output_size, density_context *const context) {
    if (context == NULL) {
        return density_make_result(DENSITY_STATE_ERROR_INVALID_CONTEXT, 0, 0, context);
    }

    if (density_compress_safe_output_size((const DENSITY_ALGORITHM) context->metadata.algorithm, input_size) > output_size) {
        return density_make_result(DENSITY_STATE_ERROR_OUTPUT_BUFFER_TOO_SMALL, 0, 0, context);
    }

    // Variables setup
    const uint8_t *in = input_buffer;
    uint8_t *out = output_buffer;
    density_algorithm_state state;
    density_algorithm_exit_status status;

    // Header
    density_header_write(&out, &context->metadata);

    // Compression
    density_algorithms_prepare_state(&state, context->dictionary.pointer);
    switch (context->metadata.algorithm) {
        case DENSITY_ALGORITHM_CHAMELEON:
            status = density_chameleon_encode(&state, &in, input_size, &out, output_size);
            break;
        case DENSITY_ALGORITHM_CHEETAH:
            status = density_cheetah_encode(&state, &in, input_size, &out, output_size);
            break;
        case DENSITY_ALGORITHM_LION:
            status = density_lion_encode(&state, &in, input_size, &out, output_size);
            break;
        default:
            return density_make_result(DENSITY_STATE_ERROR_INVALID_ALGORITHM, 0, 0, context);
    }

    // Result
    return density_make_result(density_convert_algorithm_exit_status(status), in - input_buffer, out - output_buffer, context);
}

DENSITY_WINDOWS_EXPORT density_processing_result density_decompress_prepare_context(const uint8_t *input_buffer, const uint64_t input_size, const bool is_dictionary_custom, void *(*mem_alloc)(size_t)) {
    // Variables setup
    const uint8_t* in = input_buffer;
    if (mem_alloc == NULL) {
        mem_alloc = malloc;
    }

    // Read metadata
    density_metadata metadata;
    if (!density_header_read(&in, input_size, &metadata)) {
        return density_make_result(DENSITY_STATE_ERROR_INPUT_BUFFER_TOO_SMALL, in - input_buffer, 0, NULL);
    }

    // Setup context
    density_context *const context = density_allocate_context(metadata, is_dictionary_custom, mem_alloc);
    return density_make_result(DENSITY_STATE_OK, in - input_buffer, 0, context);
}

DENSITY_WINDOWS_EXPORT density_processing_result density_decompress_with_context(const uint8_t *input_buffer, const uint64_t input_size, uint8_t *output_buffer, const uint64_t output_size, density_context *const context) {
    if (context == NULL) {
        return density_make_result(DENSITY_STATE_ERROR_INVALID_CONTEXT, 0, 0, context);
    }

    if (context->metadata.original_size > output_size) {
        return density_make_result(DENSITY_STATE_ERROR_OUTPUT_BUFFER_TOO_SMALL, 0, 0, context);
    }

    // Variables setup
    const uint8_t *in = input_buffer;
    uint8_t *out = output_buffer;
    density_algorithm_state state;
    density_algorithm_exit_status status;

    // Decompression
    density_algorithms_prepare_state(&state, context->dictionary.pointer);
    switch (context->metadata.algorithm) {
        case DENSITY_ALGORITHM_CHAMELEON:
            status = density_chameleon_decode(&state, &in, input_size, &out, output_size, context->metadata.original_size);
            break;
        case DENSITY_ALGORITHM_CHEETAH:
            status = density_cheetah_decode(&state, &in, input_size, &out, output_size);
            break;
        case DENSITY_ALGORITHM_LION:
            status = density_lion_decode(&state, &in, input_size, &out, output_size);
            break;
        default:
            return density_make_result(DENSITY_STATE_ERROR_INVALID_ALGORITHM, 0, 0, context);
    }

    // Result
    return density_make_result(density_convert_algorithm_exit_status(status), in - input_buffer, out - output_buffer, context);
}

DENSITY_WINDOWS_EXPORT density_processing_result density_compress(const uint8_t *input_buffer, const uint64_t input_size, uint8_t *output_buffer, const uint64_t output_size, const DENSITY_ALGORITHM algorithm) {
    density_processing_result result = density_compress_prepare_context(algorithm, input_size, false, malloc);
    if(result.state) {
        density_free_context(result.context, free);
        return result;
    }

    result = density_compress_with_context(input_buffer, input_size, output_buffer, output_size, result.context);
    density_free_context(result.context, free);
    return result;
}

DENSITY_WINDOWS_EXPORT density_processing_result density_decompress(const uint8_t *input_buffer, const uint64_t input_size, uint8_t *output_buffer, const uint64_t output_size) {
    density_processing_result result = density_decompress_prepare_context(input_buffer, input_size, false, malloc);
    if(result.state) {
        density_free_context(result.context, free);
        return result;
    }

    result = density_decompress_with_context(input_buffer + result.bytes_read, input_size - result.bytes_read, output_buffer, output_size, result.context);
    density_free_context(result.context, free);
    return result;
}
