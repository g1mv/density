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
    teleport->stagingMemoryLocation->originalPointer = (density_byte *) mem_alloc(size * sizeof(density_byte));
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
    mem_free(teleport->stagingMemoryLocation->originalPointer);
    mem_free(teleport->stagingMemoryLocation);
    mem_free(teleport);
}

DENSITY_FORCE_INLINE void density_memory_teleport_reset_staging(density_memory_teleport *teleport) {
    teleport->stagingMemoryLocation->pointer = teleport->stagingMemoryLocation->originalPointer;
    teleport->stagingMemoryLocation->position = 0;
}

DENSITY_FORCE_INLINE void density_memory_teleport_store(density_memory_teleport *restrict teleport, density_byte *in, const uint_fast64_t availableIn) {
    teleport->directMemoryLocation->pointer = in;
    teleport->directMemoryLocation->available_bytes = availableIn;
}

DENSITY_FORCE_INLINE uint_fast64_t density_memory_teleport_available_from_staging(density_memory_teleport *teleport) {
    return teleport->stagingMemoryLocation->position - (teleport->stagingMemoryLocation->pointer - teleport->stagingMemoryLocation->originalPointer);
}

DENSITY_FORCE_INLINE uint_fast64_t density_memory_teleport_available_from_direct(density_memory_teleport *teleport) {
    return teleport->directMemoryLocation->available_bytes;
}

DENSITY_FORCE_INLINE uint_fast64_t density_memory_teleport_available(density_memory_teleport *teleport) {
    return density_memory_teleport_available_reserved(teleport, 0);
}

DENSITY_FORCE_INLINE uint_fast64_t density_memory_teleport_available_reserved(density_memory_teleport *teleport, uint_fast64_t reserved) {
    uint_fast64_t contained = density_memory_teleport_available_from_staging(teleport) + density_memory_teleport_available_from_direct(teleport);
    if(reserved >= contained)
        return 0;
    else
        return contained - reserved;
}

DENSITY_FORCE_INLINE density_memory_location *density_memory_teleport_read(density_memory_teleport *restrict teleport, uint_fast64_t bytes) {
    return density_memory_teleport_read_reserved(teleport, bytes, 0);
}

DENSITY_FORCE_INLINE density_memory_location *density_memory_teleport_read_reserved(density_memory_teleport *restrict teleport, uint_fast64_t bytes, uint_fast64_t reserved) {
    uint_fast64_t stagingAvailableBytes = density_memory_teleport_available_from_staging(teleport);
    uint_fast64_t directAvailableBytes = density_memory_teleport_available_from_direct(teleport);

    if(stagingAvailableBytes) {
        if (bytes <= stagingAvailableBytes - reserved) {
            teleport->indirectMemoryLocation->pointer = teleport->stagingMemoryLocation->pointer;
            teleport->stagingMemoryLocation->pointer += bytes;
            teleport->indirectMemoryLocation->available_bytes = bytes;
            return teleport->indirectMemoryLocation;
        } else if (bytes <= stagingAvailableBytes + directAvailableBytes - reserved) {
            uint_fast64_t missingBytes = bytes - stagingAvailableBytes;
            memcpy(teleport->stagingMemoryLocation->originalPointer + teleport->stagingMemoryLocation->position, teleport->directMemoryLocation->pointer, missingBytes);
            teleport->indirectMemoryLocation->pointer = teleport->stagingMemoryLocation->pointer;
            teleport->indirectMemoryLocation->available_bytes = bytes;
            teleport->directMemoryLocation->pointer += missingBytes;
            teleport->directMemoryLocation->available_bytes -= missingBytes;
            density_memory_teleport_reset_staging(teleport);
            return teleport->indirectMemoryLocation;
        } else {
            memcpy(teleport->stagingMemoryLocation->originalPointer + teleport->stagingMemoryLocation->position, teleport->directMemoryLocation->pointer, directAvailableBytes);
            teleport->stagingMemoryLocation->position += directAvailableBytes;
            teleport->directMemoryLocation->pointer += directAvailableBytes;
            teleport->directMemoryLocation->available_bytes = 0;
            return NULL;
        }
    } else {
        if(bytes <= directAvailableBytes - reserved) {
            return teleport->directMemoryLocation;
        } else {
            memcpy(teleport->stagingMemoryLocation->originalPointer + teleport->stagingMemoryLocation->position, teleport->directMemoryLocation->pointer, directAvailableBytes);
            teleport->stagingMemoryLocation->position += directAvailableBytes;
            teleport->directMemoryLocation->pointer += directAvailableBytes;
            teleport->directMemoryLocation->available_bytes = 0;
            return NULL;
        }
    }
}

DENSITY_FORCE_INLINE void density_memory_teleport_copy(density_memory_teleport *restrict teleport, density_memory_location *restrict out, uint_fast64_t bytes) {
    uint_fast64_t fromStaging = 0;
    uint_fast64_t fromDirect = 0;
    uint_fast64_t stagingAvailableBytes = density_memory_teleport_available_from_staging(teleport);
    uint_fast64_t directAvailableBytes = density_memory_teleport_available_from_direct(teleport);

    if(stagingAvailableBytes) {
        if(bytes <= stagingAvailableBytes) {
            fromStaging = bytes;
        } else if(bytes <= stagingAvailableBytes + directAvailableBytes) {
            fromStaging = stagingAvailableBytes;
            fromDirect = bytes - stagingAvailableBytes;
        } else {
            fromStaging = stagingAvailableBytes;
            fromDirect = directAvailableBytes;
        }
    } else {
        if(bytes <= directAvailableBytes) {
            fromDirect = bytes;
        } else {
            fromDirect = directAvailableBytes;
        }
    }

    memcpy(out->pointer, teleport->stagingMemoryLocation->pointer, fromStaging);
    if(fromStaging == stagingAvailableBytes)
        density_memory_teleport_reset_staging(teleport);
    else
        teleport->stagingMemoryLocation->pointer += fromStaging;
    out->pointer += fromStaging;
    out->available_bytes -= fromStaging;

    memcpy(out->pointer, teleport->directMemoryLocation->pointer, fromDirect);
    teleport->directMemoryLocation->pointer += fromDirect;
    teleport->directMemoryLocation->available_bytes -= fromDirect;
    out->pointer += fromDirect;
    out->available_bytes -= fromDirect;
}

DENSITY_FORCE_INLINE void density_memory_teleport_copy_remaining(density_memory_teleport *restrict in, density_memory_location *restrict out) {
    return density_memory_teleport_copy(in, out, density_memory_teleport_available(in));
}