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
 * 11/10/13 17:56
 */

#include "main_header.h"

SSC_FORCE_INLINE uint_fast32_t ssc_main_header_read(ssc_byte_buffer *restrict in, ssc_main_header *restrict header) {
    ssc_byte *pointer = in->pointer + in->position;
    header->version[0] = *(pointer);
    header->version[1] = *(pointer + 1);
    header->version[2] = *(pointer + 2);
    header->compressionMode = *(pointer + 3);
    header->blockType = *(pointer + 4);
    in->position += sizeof(ssc_main_header);
    return sizeof(ssc_main_header);
}

SSC_FORCE_INLINE uint_fast32_t ssc_main_header_write(ssc_byte_buffer *restrict out, const SSC_COMPRESSION_MODE compressionMode, SSC_BLOCK_TYPE blockType) {
    ssc_byte *pointer = out->pointer + out->position;
    *(pointer) = SSC_MAJOR_VERSION;
    *(pointer + 1) = SSC_MINOR_VERSION;
    *(pointer + 2) = SSC_REVISION;
    *(pointer + 3) = compressionMode;
    *(pointer + 4) = blockType;
    out->position += sizeof(ssc_main_header);
    return sizeof(ssc_main_header);
}