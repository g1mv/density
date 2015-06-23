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

DENSITY_WINDOWS_EXPORT DENSITY_FORCE_INLINE density_buffer_processing_result density_buffer_compress(const uint8_t *restrict input_buffer, const uint_fast64_t input_size, uint8_t *restrict output_buffer, const uint_fast64_t output_size, const DENSITY_COMPRESSION_MODE compression_mode, const DENSITY_BLOCK_TYPE block_type, void *(*mem_alloc)(size_t), void (*mem_free)(void *)) {
    const uint8_t *in = input_buffer;
    uint8_t *out = output_buffer;

    density_chameleon_encode_unrestricted(&in, input_size, &out);

    density_buffer_processing_result result;
    result.state = DENSITY_BUFFER_STATE_OK;
    result.bytesRead = in - input_buffer;
    result.bytesWritten = out - output_buffer;
    return result;
}

DENSITY_WINDOWS_EXPORT DENSITY_FORCE_INLINE density_buffer_processing_result density_buffer_decompress(const uint8_t *restrict input_buffer, const uint_fast64_t input_size, uint8_t *restrict output_buffer, const uint_fast64_t output_size, void *(*mem_alloc)(size_t), void (*mem_free)(void *)) {
    const uint8_t *in = input_buffer;
    uint8_t *out = output_buffer;

    bool valid = density_chameleon_decode_bulk_unrestricted(&in, input_size, &out);

    density_buffer_processing_result result;
    result.state = valid ? DENSITY_BUFFER_STATE_OK : DENSITY_BUFFER_STATE_ERROR_DURING_PROCESSING;
    result.bytesRead = in - input_buffer;
    result.bytesWritten = out - output_buffer;
    return result;
}
