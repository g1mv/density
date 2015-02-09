/*
 * Centaurean Density
 *
 * Copyright (c) 2015, Guillaume Voirin
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
 * 5/02/15 20:57
 *
 * ------------------
 * Argonaut algorithm
 * ------------------
 *
 * Author(s)
 * Guillaume Voirin (https://github.com/gpnuma)
 *
 * Description
 * Rank, predictions and dictionary hash algorithm
 */

#include "kernel_argonaut_signature.h"

DENSITY_FORCE_INLINE uint_fast8_t density_kernel_argonaut_signature_read(density_argonaut_signature signature, uint_fast8_t shift) {
    uint_fast8_t group = shift >> 6;
    uint_fast8_t localShift = shift & 63;
    if(density_likely(localShift < 62)) {
        return ((signature.elements[group] >> localShift) & 0x7);
    } else if(localShift == 62) {
        return (((signature.elements[group] >> localShift) & 0x3) | ((signature.elements[group + 1] & 0x1) << 2));
    } else if(localShift == 63) {
        return (((signature.elements[group] >> localShift) & 0x1) | ((signature.elements[group + 1] & 0x3) << 1));
    }
}

DENSITY_FORCE_INLINE void density_kernel_argonaut_signature_write(density_argonaut_signature signature, uint_fast8_t shift, uint_fast8_t value) {
    uint_fast8_t group = shift >> 6;
    uint_fast8_t localShift = shift & 63;
    if(density_likely(localShift < 62)) {
        signature.elements[group] |= (value << localShift);
        return;
    } else if(localShift == 62) {
        signature.elements[group] |= (value << localShift);
        signature.elements[group + 1] |= (value >> 2);
        return;
    } else if(localShift == 63) {
        signature.elements[group] |= (value << localShift);
        signature.elements[group + 1] |= (value >> 1);
        return;
    }
}
