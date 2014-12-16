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
 * 16/12/14 17:12
 */

#include <string.h>
#include "teleport.h"

DENSITY_FORCE_INLINE void density_teleport_open(density_teleport restrict *teleport) {
    teleport->source = DENSITY_TELEPORT_SOURCE_STAGING;
    teleport->state = DENSITY_TELEPORT_STATE_STAGING;
    teleport->staging = (density_memory_location *) malloc(sizeof(density_memory_location));
    teleport->staging->pointer = (density_byte *) malloc(DENSITY_TELEPORT_BUFFER_SIZE * sizeof(density_byte));
    teleport->staging->available = DENSITY_TELEPORT_BUFFER_SIZE;
    teleport->stagingPosition = 0;
}

DENSITY_FORCE_INLINE void density_teleport_close(density_teleport *teleport) {
    free(teleport->staging->pointer);
    free(teleport->staging);
}

DENSITY_FORCE_INLINE uint_fast64_t density_teleport_get(density_teleport restrict *teleport, uint_fast64_t minimumBytes, density_memory_location restrict *outBuffer) {
    uint_fast64_t stagingFree;
    switch (teleport->source) {
        case DENSITY_TELEPORT_SOURCE_STAGING:
            stagingFree = DENSITY_TELEPORT_BUFFER_SIZE - teleport->stagingPosition;
            if (teleport->memory->available > stagingFree) {
                memcpy(teleport->staging->pointer + teleport->stagingPosition, teleport->memory, stagingFree);
                teleport->staging->available = DENSITY_TELEPORT_BUFFER_SIZE;
                outBuffer = teleport->staging;
            }
            break;
        default:
            if (teleport->memory->available < minimumBytes) {
                teleport->source = DENSITY_TELEPORT_SOURCE_STAGING;
                teleport->stagingPosition = 0;
            }
            break;
    }

    if (teleport->staging->available) if (teleport->staging->available >= minimumBytes)
        return teleport->staging;
    if (teleport->memory->available >= minimumBytes)
        return teleport->memory;

}

DENSITY_FORCE_INLINE void density_teleport_put(density_teleport restrict *teleport, density_memory_location *data) {

}