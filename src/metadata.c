/*
 * Centaurean Density
 * http://www.libssc.net
 *
 * Copyright (c) 2013, Guillaume Voirin
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Centaurean nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * 18/10/13 22:12
 */

#include "metadata.h"

DENSITY_FORCE_INLINE uint_fast64_t density_metadata_main_structure_overhead(const DENSITY_ENCODE_OUTPUT_TYPE encodeOutputType) {
    switch (encodeOutputType) {
        case DENSITY_ENCODE_OUTPUT_TYPE_DEFAULT:
            return sizeof(density_main_header) + sizeof(density_main_footer);
        case DENSITY_ENCODE_OUTPUT_TYPE_WITHOUT_FOOTER:
            return sizeof(density_main_header);
        case DENSITY_ENCODE_OUTPUT_TYPE_WITHOUT_HEADER:
            return sizeof(density_main_header);
        case DENSITY_ENCODE_OUTPUT_TYPE_WITHOUT_HEADER_NOR_FOOTER:
            return 0;
    }
}

DENSITY_FORCE_INLINE uint_fast64_t density_metadata_max_block_structure_overhead(const uint_fast64_t length, const DENSITY_COMPRESSION_MODE compressionMode) {
    switch(compressionMode) {
        case DENSITY_COMPRESSION_MODE_COPY:
            return (1 + length / DENSITY_PREFERRED_COPY_BLOCK_SIZE) * (sizeof(density_block_header) + sizeof(density_block_footer));
        case DENSITY_COMPRESSION_MODE_CHAMELEON_ALGORITHM:
            return (1 + length / (DENSITY_CHAMELEON_PREFERRED_BLOCK_SIGNATURES * (sizeof(density_chameleon_signature) + bitsizeof(density_chameleon_signature) * sizeof(uint16_t)))) * (sizeof(density_block_header) + sizeof(density_mode_marker) + sizeof(density_block_footer));
        case DENSITY_COMPRESSION_MODE_MANDALA_ALGORITHM:
            return (1 + length / (DENSITY_MANDALA_PREFERRED_BLOCK_SIGNATURES * sizeof(density_mandala_signature))) * (sizeof(density_block_header) + sizeof(density_mode_marker) + sizeof(density_block_footer));
    }
}

DENSITY_FORCE_INLINE uint_fast64_t density_metadata_max_compressed_length(const uint_fast64_t length, const DENSITY_COMPRESSION_MODE compressionMode, const DENSITY_ENCODE_OUTPUT_TYPE encodeOutputType) {
    return density_metadata_main_structure_overhead(encodeOutputType) + density_metadata_max_block_structure_overhead(length, compressionMode) + length;
}

DENSITY_FORCE_INLINE uint_fast64_t density_metadata_max_decompressed_length(const uint_fast64_t length, const DENSITY_COMPRESSION_MODE compressionMode) {
    switch(compressionMode) {
        case DENSITY_COMPRESSION_MODE_COPY:
            return length;
        case DENSITY_COMPRESSION_MODE_CHAMELEON_ALGORITHM:
            return (sizeof(uint32_t) * bitsizeof(density_chameleon_signature)) * (1 + length / (sizeof(density_mandala_signature) + bitsizeof(density_chameleon_signature) * sizeof(uint16_t)));
        case DENSITY_COMPRESSION_MODE_MANDALA_ALGORITHM:
            return (sizeof(uint32_t) * (bitsizeof(density_mandala_signature) >> 1))  * (1 + length / sizeof(density_mandala_signature));
    }
}
