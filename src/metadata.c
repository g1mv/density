/*
 * Centaurean libssc
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

SSC_FORCE_INLINE uint_fast64_t ssc_metadata_structure_overhead() {
    return sizeof(ssc_main_header) + sizeof(ssc_main_footer);
}

SSC_FORCE_INLINE uint_fast64_t ssc_metadata_block_structure_overhead(const uint_fast64_t length) {
    return (1 + length / (SSC_PREFERRED_BLOCK_SIGNATURES * sizeof(uint16_t) * 8 * 8/*sizeof(ssc_hash_signature)*/)) * (sizeof(ssc_block_header) + sizeof(ssc_mode_marker) + sizeof(ssc_block_footer));
}

SSC_FORCE_INLINE uint_fast64_t ssc_metadata_max_compressed_length(const uint_fast64_t length, const SSC_COMPRESSION_MODE mode, const bool includeStructure) {
    uint_fast64_t headerFooterLength = (includeStructure ? ssc_metadata_structure_overhead() : 0);
    switch(mode) {
        default:
            return headerFooterLength + length;

        case SSC_COMPRESSION_MODE_CHAMELEON:
            return headerFooterLength + ssc_metadata_block_structure_overhead(length) + length;

        case SSC_COMPRESSION_MODE_DUAL_PASS_CHAMELEON:
            return headerFooterLength + ssc_metadata_block_structure_overhead(ssc_metadata_block_structure_overhead(length) + length) + length;
    }
}

SSC_FORCE_INLINE uint_fast64_t ssc_metadata_max_decompressed_length(const uint_fast64_t length, const SSC_COMPRESSION_MODE mode, const bool includeStructure) {
    uint_fast64_t headerFooterLength = (includeStructure ? ssc_metadata_structure_overhead() : 0);
    uint_fast64_t intermediate;
    switch(mode) {
        default:
            return length - headerFooterLength;

        case SSC_COMPRESSION_MODE_CHAMELEON:
            return (length - ssc_metadata_block_structure_overhead(length)) << (1 - headerFooterLength);

        case SSC_COMPRESSION_MODE_DUAL_PASS_CHAMELEON:
            intermediate = (length - ssc_metadata_block_structure_overhead(length)) << 1;
            return (intermediate - ssc_metadata_block_structure_overhead(intermediate)) << (1 - headerFooterLength);
    }
}
