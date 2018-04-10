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
 * 11/10/13 17:56
 */

#include "../api.h"
#include "header.h"

DENSITY_WINDOWS_EXPORT bool density_header_read(const uint8_t **DENSITY_RESTRICT in, const uint_fast64_t in_size, density_metadata *DENSITY_RESTRICT metadata) {
    if (in_size < DENSITY_HEADER_BASE_SIZE) {
        return false;
    }

    metadata->version_major = *((*in)++);
    metadata->version_minor = *((*in)++);
    metadata->version_revision = *((*in)++);
    metadata->algorithm = *((*in)++);
    const uint8_t original_size_bytes = *((*in)++);
    if (in_size < DENSITY_HEADER_BASE_SIZE + original_size_bytes) {
        return false;
    }

    if (original_size_bytes) {
        metadata->original_size = 0;
        DENSITY_ENDIAN_MEMCPY_AND_CORRECT_64(DENSITY_MEMCPY, &metadata->original_size, *in, original_size_bytes);
        (*in) += original_size_bytes;
    } else {
        metadata->original_size = 0;
    }

    return true;
}

DENSITY_WINDOWS_EXPORT void density_header_write(uint8_t **DENSITY_RESTRICT out, density_metadata *DENSITY_RESTRICT metadata) {
    *((*out)++) = metadata->version_major;
    *((*out)++) = metadata->version_minor;
    *((*out)++) = metadata->version_revision;
    *((*out)++) = metadata->algorithm;
    if (metadata->original_size) {
        const uint8_t original_size_bytes = DENSITY_HEADER_ORIGINAL_SIZE_BYTES(metadata->original_size);
        *((*out)++) = original_size_bytes;
        DENSITY_ENDIAN_CORRECT_64_AND_MEMCPY(DENSITY_MEMCPY, *out, &metadata->original_size, original_size_bytes);
        (*out) += original_size_bytes;
    } else {
        *((*out)++) = 0;
    }
}
