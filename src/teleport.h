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

#ifndef DENSITY_TELEPORT_H
#define DENSITY_TELEPORT_H

#include <string.h>

#include "globals.h"
#include "density_api.h"

#define DENSITY_TELEPORT_BUFFER_SIZE    16384

typedef enum {
    DENSITY_TELEPORT_SOURCE_INDIRECT_ACCESS,
    DENSITY_TELEPORT_SOURCE_DIRECT_ACCESS
} DENSITY_TELEPORT_SOURCE;

typedef struct {
    density_byte *pointer;
    uint_fast64_t available_bytes;
} density_memory_location;

typedef struct {
    density_byte *pointer;
    uint_fast64_t position;
} density_staging_memory_location;

typedef struct {
    DENSITY_TELEPORT_SOURCE source;
    density_staging_memory_location *stagingMemoryLocation;
    density_memory_location *indirectMemoryLocation;
    density_memory_location *directMemoryLocation;
} density_teleport;

density_teleport *density_teleport_allocate(uint_fast64_t);

density_memory_location *density_teleport_get(density_teleport *, uint_fast64_t);

void density_teleport_put(density_teleport *, density_memory_location *data);

void density_teleport_free(density_teleport *);

#endif
