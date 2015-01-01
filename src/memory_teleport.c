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

#include "memory_teleport.h"
#include "memory_location.h"

DENSITY_FORCE_INLINE density_memory_teleport *density_memory_teleport_allocate(uint_fast64_t size, void *(*mem_alloc)(size_t)) {
    density_memory_teleport *teleport = (density_memory_teleport *) mem_alloc(sizeof(density_memory_teleport));
    teleport->stagingMemoryLocation = (density_staging_memory_location *) mem_alloc(sizeof(density_staging_memory_location));
    teleport->stagingMemoryLocation->pointer = (density_byte *) mem_alloc(size * sizeof(density_byte));
    density_memory_teleport_reset_staging(teleport);
    teleport->indirectMemoryLocation = (density_memory_location *) mem_alloc(sizeof(density_memory_location));
    teleport->indirectMemoryLocation->available_bytes = 0;
    teleport->directMemoryLocation = (density_memory_location *) mem_alloc(sizeof(density_memory_location));
    teleport->directMemoryLocation->available_bytes = 0;
    return teleport;
}

DENSITY_FORCE_INLINE void density_memory_teleport_free(density_memory_teleport *teleport, void (*mem_free)(void *)) {
    mem_free(teleport->indirectMemoryLocation);
    mem_free(teleport->directMemoryLocation);
    mem_free(teleport->stagingMemoryLocation->pointer);
    mem_free(teleport->stagingMemoryLocation);
    mem_free(teleport);
}

DENSITY_FORCE_INLINE void density_memory_teleport_reset_staging(density_memory_teleport *teleport) {
    teleport->source = DENSITY_MEMORY_TELEPORT_INPUT_SOURCE_DIRECT_ACCESS;
    teleport->stagingMemoryLocation->position = 0;
}

DENSITY_FORCE_INLINE void density_memory_teleport_store(density_memory_teleport *restrict teleport, density_byte *in, const uint_fast64_t availableIn) {
    teleport->directMemoryLocation->pointer = in;
    teleport->directMemoryLocation->available_bytes = availableIn;
}

DENSITY_FORCE_INLINE density_memory_location *density_memory_teleport_read(density_memory_teleport *restrict teleport, uint_fast64_t bytes) {
    uint_fast64_t missingBytes;
    uint_fast64_t availableBytes = teleport->directMemoryLocation->available_bytes;
    switch (teleport->source) {
        case DENSITY_MEMORY_TELEPORT_INPUT_SOURCE_INDIRECT_ACCESS:
            missingBytes = bytes - teleport->stagingMemoryLocation->position;
            if (availableBytes >= missingBytes) {
                memcpy(teleport->stagingMemoryLocation->pointer + teleport->stagingMemoryLocation->position, teleport->directMemoryLocation->pointer, missingBytes);
                teleport->indirectMemoryLocation->pointer = teleport->stagingMemoryLocation->pointer;
                teleport->indirectMemoryLocation->available_bytes = bytes;
                teleport->directMemoryLocation->pointer += missingBytes;
                teleport->directMemoryLocation->available_bytes -= missingBytes;
                density_memory_teleport_reset_staging(teleport);
                return teleport->indirectMemoryLocation;
            } else {
                memcpy(teleport->stagingMemoryLocation->pointer + teleport->stagingMemoryLocation->position, teleport->directMemoryLocation->pointer, availableBytes);
                teleport->stagingMemoryLocation->position += availableBytes;
                teleport->directMemoryLocation->pointer += availableBytes;
                teleport->directMemoryLocation->available_bytes = 0;
                return NULL;
            }
        case DENSITY_MEMORY_TELEPORT_INPUT_SOURCE_DIRECT_ACCESS:
            if (availableBytes >= bytes)
                return teleport->directMemoryLocation;
            else {
                memcpy(teleport->stagingMemoryLocation->pointer, teleport->directMemoryLocation->pointer, availableBytes);
                teleport->stagingMemoryLocation->position += availableBytes;
                teleport->directMemoryLocation->pointer += availableBytes;
                teleport->directMemoryLocation->available_bytes = 0;
                teleport->source = DENSITY_MEMORY_TELEPORT_INPUT_SOURCE_INDIRECT_ACCESS;
                return NULL;
            }
    }
}

DENSITY_FORCE_INLINE uint_fast64_t density_memory_teleport_available(density_memory_teleport *teleport) {
    return teleport->directMemoryLocation->available_bytes + teleport->stagingMemoryLocation->position;
}

DENSITY_FORCE_INLINE void density_memory_teleport_copy(density_memory_teleport *in, density_memory_location *out, uint_fast64_t bytes) {
    uint_fast64_t fromStaging = 0;
    uint_fast64_t fromDirect = 0;
    switch (in->source) {
        case DENSITY_MEMORY_TELEPORT_INPUT_SOURCE_INDIRECT_ACCESS:
            if (bytes < in->stagingMemoryLocation->position) {
                fromStaging = bytes;
            } else {
                fromStaging = in->stagingMemoryLocation->position;
                fromDirect = bytes - in->stagingMemoryLocation->position;
                if (fromDirect > in->directMemoryLocation->available_bytes)
                    fromDirect = in->directMemoryLocation->available_bytes;
            }

            memcpy(out->pointer, in->stagingMemoryLocation->pointer, fromStaging);
            in->stagingMemoryLocation->position -= fromStaging;
            out->pointer += fromStaging;
            out->available_bytes -= fromStaging;

            if (fromDirect) {
                memcpy(out->pointer, in->directMemoryLocation->pointer, fromDirect);
                in->directMemoryLocation->pointer += fromDirect;
                in->directMemoryLocation->available_bytes -= fromDirect;
                out->pointer += fromDirect;
                out->available_bytes -= fromDirect;
                density_memory_teleport_reset_staging(in);
            }
            return;

        case DENSITY_MEMORY_TELEPORT_INPUT_SOURCE_DIRECT_ACCESS:
            if (bytes > in->directMemoryLocation->available_bytes)
                bytes = in->directMemoryLocation->available_bytes;

            memcpy(out->pointer, in->directMemoryLocation->pointer, bytes);
            in->directMemoryLocation->pointer += bytes;
            in->directMemoryLocation->available_bytes -= bytes;
            out->pointer += bytes;
            out->available_bytes -= bytes;
            return;
    }
}