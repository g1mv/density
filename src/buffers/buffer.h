/*
 * Centaurean Density
 *
 * Copyright (c) 2015, Guillaume Voirin
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
 * 3/02/15 19:51
 */

#ifndef DENSITY_BUFFER_H
#define DENSITY_BUFFER_H

#include "../globals.h"
#include "../density_api.h"
#include "../structure/header.h"
#include "../core/chameleon/chameleon_encode.h"
#include "../core/chameleon/chameleon_decode.h"
#include "../core/cheetah/cheetah_encode.h"
#include "../core/cheetah/cheetah_decode.h"
#include "../core/lion/lion_encode.h"
#include "../core/lion/lion_decode.h"

DENSITY_WINDOWS_EXPORT const uint_fast64_t density_buffer_compress_safe_size(const uint_fast64_t);

DENSITY_WINDOWS_EXPORT const uint_fast64_t density_buffer_decompress_safe_size(const uint_fast64_t);

DENSITY_WINDOWS_EXPORT const density_buffer_processing_result density_buffer_compress(const uint8_t *, const uint_fast64_t, uint8_t *, const uint_fast64_t, const DENSITY_COMPRESSION_MODE);

DENSITY_WINDOWS_EXPORT const density_buffer_processing_result density_buffer_decompress(const uint8_t *, const uint_fast64_t, uint8_t *, const uint_fast64_t);


#endif