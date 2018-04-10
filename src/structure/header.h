/*
 * Centaurean Density
 *
 * Copyright (c) 2013, Guillaume Voirin
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
 * 11/10/13 02:06
 */

#ifndef DENSITY_STRUCTURE_HEADER_H
#define DENSITY_STRUCTURE_HEADER_H

#include <stdio.h>
#include <time.h>

#include "../api.h"
#include "../globals.h"

#define DENSITY_HEADER_BASE_SIZE    5
#define DENSITY_HEADER_ORIGINAL_SIZE_BYTES(ORIGINAL_SIZE)    (8 - (DENSITY_CLZ(ORIGINAL_SIZE) / 8))

DENSITY_WINDOWS_EXPORT bool density_header_read(const uint8_t ** DENSITY_RESTRICT_DECLARE, uint_fast64_t, density_metadata * DENSITY_RESTRICT_DECLARE);

DENSITY_WINDOWS_EXPORT void density_header_write(uint8_t ** DENSITY_RESTRICT_DECLARE, density_metadata * DENSITY_RESTRICT_DECLARE);

#endif
