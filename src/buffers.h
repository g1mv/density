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
 * 18/10/13 22:37
 */

#ifndef SSC_BUFFERS_H
#define SSC_BUFFERS_H

#include "globals.h"
#include "stream.h"
#include "metadata.h"

SSC_BUFFERS_STATE ssc_buffers_max_compressed_length(uint_fast64_t *, uint_fast64_t, const SSC_COMPRESSION_MODE);
SSC_BUFFERS_STATE ssc_buffers_compress(uint_fast64_t*, uint8_t *, uint_fast64_t, uint8_t *, uint_fast64_t, const SSC_COMPRESSION_MODE, const SSC_ENCODE_OUTPUT_TYPE, const SSC_BLOCK_TYPE, void *(*mem_alloc)(size_t), void (*mem_free)(void *));
SSC_BUFFERS_STATE ssc_buffers_decompress(uint_fast64_t *, ssc_main_header*, uint8_t *, uint_fast64_t, uint8_t *, uint_fast64_t, void *(*mem_alloc)(size_t), void (*mem_free)(void *));

#endif