/*
 * Centaurean Density
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
 * 23/12/14 17:01
 */

#include "teleport.h"

DENSITY_FORCE_INLINE density_teleport *density_teleport_allocate(uint_fast64_t size) {
    density_teleport *teleport = (density_teleport *) malloc(sizeof(density_teleport));
    teleport->source = DENSITY_TELEPORT_INPUT_SOURCE_DIRECT_ACCESS;
    teleport->stagingMemoryLocation = (density_staging_memory_location *) malloc(sizeof(density_staging_memory_location));
    teleport->stagingMemoryLocation->pointer = (density_byte *) malloc(size * sizeof(density_byte));
    teleport->stagingMemoryLocation->position = 0;
    teleport->directMemoryLocation = (density_memory_location *) malloc(sizeof(density_memory_location));
    teleport->indirectMemoryLocation = (density_memory_location *) malloc(sizeof(density_memory_location));
    return teleport;
}

DENSITY_FORCE_INLINE void density_teleport_free(density_teleport *teleport) {
    free(teleport->indirectMemoryLocation);
    free(teleport->directMemoryLocation);
    free(teleport->stagingMemoryLocation->pointer);
    free(teleport->stagingMemoryLocation);
    free(teleport);
}

DENSITY_FORCE_INLINE void density_teleport_store(density_teleport *restrict teleport, density_byte *in, const uint_fast64_t availableIn) {
    teleport->directMemoryLocation->pointer = in;
    teleport->directMemoryLocation->available_bytes = availableIn;
}

DENSITY_FORCE_INLINE density_memory_location *density_teleport_access(density_teleport *restrict teleport, uint_fast64_t bytes) {
    uint_fast64_t missingBytes;
    switch (teleport->source) {
        case DENSITY_TELEPORT_INPUT_SOURCE_INDIRECT_ACCESS:
            missingBytes = bytes - teleport->stagingMemoryLocation->position;
            if (teleport->directMemoryLocation->available_bytes >= missingBytes) {
                memcpy(teleport->stagingMemoryLocation->pointer + teleport->stagingMemoryLocation->position, teleport->directMemoryLocation->pointer, missingBytes);
                teleport->indirectMemoryLocation->pointer = teleport->stagingMemoryLocation->pointer;
                teleport->indirectMemoryLocation->available_bytes = bytes;
                teleport->source = DENSITY_TELEPORT_INPUT_SOURCE_DIRECT_ACCESS;
                return teleport->indirectMemoryLocation;
            } else {
                memcpy(teleport->stagingMemoryLocation->pointer + teleport->stagingMemoryLocation->position, teleport->directMemoryLocation->pointer, teleport->directMemoryLocation->available_bytes);
                teleport->stagingMemoryLocation->position += teleport->directMemoryLocation->available_bytes;
                return NULL;
            }
        case DENSITY_TELEPORT_INPUT_SOURCE_DIRECT_ACCESS:
            if (teleport->directMemoryLocation->available_bytes >= bytes)
                return teleport->directMemoryLocation;
            else {
                memcpy(teleport->stagingMemoryLocation->pointer, teleport->directMemoryLocation->pointer, teleport->directMemoryLocation->available_bytes);
                teleport->stagingMemoryLocation->position += teleport->directMemoryLocation->available_bytes;
                teleport->source = DENSITY_TELEPORT_INPUT_SOURCE_INDIRECT_ACCESS;
                return NULL;
            }
    }
}

DENSITY_FORCE_INLINE uint_fast64_t density_teleport_available(density_teleport *teleport) {
    return teleport->directMemoryLocation->available_bytes + teleport->stagingMemoryLocation->position;
}

DENSITY_FORCE_INLINE void density_teleport_copy(density_teleport *in, density_memory_location *out, uint_fast64_t bytes) {
    if (bytes < in->stagingMemoryLocation->position) {
        memcpy(out->pointer, in->stagingMemoryLocation->pointer, bytes);
        in->stagingMemoryLocation->position += bytes;
        out->pointer += bytes;
        out->available_bytes -= bytes;
    } else if (bytes < density_teleport_available(in)) {
        memcpy(out->pointer, in->stagingMemoryLocation->pointer, in->stagingMemoryLocation->position);
        out->pointer += in->stagingMemoryLocation->position;
        out->available_bytes -= in->stagingMemoryLocation->position;
        uint_fast64_t remaining = bytes - in->stagingMemoryLocation->position;
        in->stagingMemoryLocation->position = 0;
        memcpy(out->pointer, in->directMemoryLocation->pointer, remaining);
        out->pointer += remaining;
        out->available_bytes -= remaining;
        in->directMemoryLocation->pointer += remaining;
        in->directMemoryLocation->available_bytes = 0;
        in->source = DENSITY_TELEPORT_INPUT_SOURCE_DIRECT_ACCESS;
    } else {
        memcpy(out->pointer, in->stagingMemoryLocation->pointer, in->stagingMemoryLocation->position);
        memcpy(out->pointer + in->stagingMemoryLocation->position, in->directMemoryLocation->pointer, in->directMemoryLocation->available_bytes);
        uint_fast64_t shift = in->stagingMemoryLocation->position + in->directMemoryLocation->available_bytes;
        out->pointer += shift;
        out->available_bytes -= shift;
        in->stagingMemoryLocation->position = 0;
        in->directMemoryLocation->pointer += in->directMemoryLocation->available_bytes;
        in->directMemoryLocation->available_bytes = 0;
        in->source = DENSITY_TELEPORT_INPUT_SOURCE_DIRECT_ACCESS;
    }
}