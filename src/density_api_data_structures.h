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
 * 25/12/14 17:08
 */

#ifndef DENSITY_API_DATA_STRUCTURES_H
#define DENSITY_API_DATA_STRUCTURES_H

#include <stdint.h>
#include <stdbool.h>

/***********************************************************************************************************************
 *                                                                                                                     *
 * Structures useful for the API                                                                                       *
 *                                                                                                                     *
 ***********************************************************************************************************************/


typedef uint8_t density_byte;
typedef bool density_bool;

typedef enum {
    DENSITY_COMPRESSION_MODE_COPY = 0,
    DENSITY_COMPRESSION_MODE_CHAMELEON_ALGORITHM = 1,
    DENSITY_COMPRESSION_MODE_MANDALA_ALGORITHM = 2,
} DENSITY_COMPRESSION_MODE;

typedef enum {
    DENSITY_BLOCK_TYPE_DEFAULT = 0,                                     // Standard, no integrity check
    DENSITY_BLOCK_TYPE_WITH_HASHSUM_INTEGRITY_CHECK = 1                 // Add data integrity check to the stream
} DENSITY_BLOCK_TYPE;

typedef enum {
    DENSITY_STREAM_STATE_READY = 0,                                     // Awaiting further instructions (new action or adding data to the input buffer)
    DENSITY_STREAM_STATE_STALL_ON_INPUT,                                // there is not enough space left in the input buffer to continue
    DENSITY_STREAM_STATE_STALL_ON_OUTPUT,                               // there is not enough space left in the output buffer to continue
    DENSITY_STREAM_STATE_ERROR_OUTPUT_BUFFER_TOO_SMALL,                 // output buffer size is too small
    DENSITY_STREAM_STATE_ERROR_INVALID_INTERNAL_STATE                   // error during processing
} DENSITY_STREAM_STATE;

typedef struct {
    density_byte majorVersion;
    density_byte minorVersion;
    density_byte revision;
    DENSITY_COMPRESSION_MODE compressionMode;
    DENSITY_BLOCK_TYPE blockType;
} density_stream_header_information;

typedef struct {
    void *in;
    uint_fast64_t *in_total_read;

    void *out;
    uint_fast64_t *out_total_written;

    void *internal_state;
} density_stream;

#endif

