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
 * 24/10/13 11:57
 */

#ifndef SSC_CHAMELEON_H
#define SSC_CHAMELEON_H

#include "globals.h"

#ifdef SSC_CHAMELEON_1P
#define SSC_CHAMELEON_SUFFIX                                        1p
#define SSC_CHAMELEON_HASH_OFFSET_BASIS                             (uint32_t)2885564586
#else
#define SSC_CHAMELEON_SUFFIX                                        2p
#define SSC_CHAMELEON_HASH_OFFSET_BASIS                             (uint32_t)2166115717
#endif

#define PASTER(x,y) x ## _ ## y
#define EVALUATOR(x,y)  PASTER(x,y)
#define CHAMELEON_NAME(function) EVALUATOR(function, SSC_CHAMELEON_SUFFIX)

#define SSC_CHAMELEON_HASH_BITS                                     16
#define SSC_CHAMELEON_HASH_PRIME                                    16777619

typedef uint64_t                                                    ssc_hash_signature;

#define SSC_CHAMELEON_HASH_ALGORITHM(hash, value)                   hash = SSC_CHAMELEON_HASH_OFFSET_BASIS;\
                                                                    hash ^= value;\
                                                                    hash *= SSC_CHAMELEON_HASH_PRIME;\
                                                                    hash = (hash >> (32 - SSC_CHAMELEON_HASH_BITS)) ^ (hash & ((1 << SSC_CHAMELEON_HASH_BITS) - 1));

#endif
