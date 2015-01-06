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
 * 11/10/13 17:56
 */

#include "main_header.h"

DENSITY_FORCE_INLINE uint_fast32_t density_main_header_read(density_memory_location *restrict in, density_main_header *restrict header) {
    density_byte *pointer = in->pointer;
    header->version[0] = *(pointer);
    header->version[1] = *(pointer + 1);
    header->version[2] = *(pointer + 2);
    header->compressionMode = *(pointer + 3);
    header->blockType = *(pointer + 4);
    header->parameters = *(density_main_header_parameters *) (pointer + 8);

    in->pointer += sizeof(density_main_header);
    in->available_bytes -= sizeof(density_main_header);
    return sizeof(density_main_header);
}

DENSITY_FORCE_INLINE uint_fast32_t density_main_header_write(density_memory_location *restrict out, const DENSITY_COMPRESSION_MODE compressionMode, const DENSITY_BLOCK_TYPE blockType, const density_main_header_parameters parameters) {
    density_byte *pointer = out->pointer;
    *(pointer) = DENSITY_MAJOR_VERSION;
    *(pointer + 1) = DENSITY_MINOR_VERSION;
    *(pointer + 2) = DENSITY_REVISION;
    *(pointer + 3) = compressionMode;
    *(pointer + 4) = blockType;
    *(uint64_t * )(pointer + 8) = DENSITY_LITTLE_ENDIAN_64(parameters.as_uint64_t);

    out->pointer += sizeof(density_main_header);
    out->available_bytes -= sizeof(density_main_header);
    return sizeof(density_main_header);
}