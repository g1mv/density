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
 * 01/11/13 13:39
 */

#include "globals.h"

DENSITY_WINDOWS_EXPORT uint8_t density_version_major() {
    return DENSITY_MAJOR_VERSION;
}

DENSITY_WINDOWS_EXPORT uint8_t density_version_minor() {
    return DENSITY_MINOR_VERSION;
}

DENSITY_WINDOWS_EXPORT uint8_t density_version_revision() {
    return DENSITY_REVISION;
}

DENSITY_WINDOWS_EXPORT DENSITY_FORCE_INLINE uint16_t density_read_2(void *input_pointer) {
    uint16_t result;
    DENSITY_MEMCPY(&result, input_pointer, sizeof(uint16_t));
    return result;
}

DENSITY_WINDOWS_EXPORT DENSITY_FORCE_INLINE uint32_t density_read_4(void *input_pointer) {
    uint32_t result;
    DENSITY_MEMCPY(&result, input_pointer, sizeof(uint32_t));
    return result;
}

DENSITY_WINDOWS_EXPORT DENSITY_FORCE_INLINE uint64_t density_read_8(void *input_pointer) {
    uint64_t result;
    DENSITY_MEMCPY(&result, input_pointer, sizeof(uint64_t));
    return result;
}

DENSITY_WINDOWS_EXPORT DENSITY_FORCE_INLINE void density_write_2(void *output_pointer, const uint16_t input) {
    DENSITY_MEMCPY(output_pointer, &input, sizeof(uint16_t));
}

DENSITY_WINDOWS_EXPORT DENSITY_FORCE_INLINE void density_write_4(void *output_pointer, const uint32_t input) {
    DENSITY_MEMCPY(output_pointer, &input, sizeof(uint32_t));
}

DENSITY_WINDOWS_EXPORT DENSITY_FORCE_INLINE void density_write_8(void *output_pointer, const uint64_t input) {
    DENSITY_MEMCPY(output_pointer, &input, sizeof(uint64_t));
}