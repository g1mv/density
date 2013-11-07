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
 * 24/10/13 12:01
 */

#ifdef SSC_CHAMELEON_CODE_INCLUSION

#include "kernel_chameleon_encode.h"

SSC_FORCE_INLINE void ssc_hash_encode_writeToSignature(ssc_chameleon_encode_state *state) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    *(state->signature) |= ((uint64_t) 1) << state->shift;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    *(state->signature) |= ((uint64_t) 1) << ((56 - (state->shift & ~0x7)) + (state->shift & 0x7));
#endif
}

SSC_FORCE_INLINE SSC_KERNEL_ENCODE_STATE ssc_hash_encode_prepareNewBlock(ssc_byte_buffer *restrict out, ssc_chameleon_encode_state *restrict state, const uint_fast32_t minimumLookahead) {
    if (out->position + minimumLookahead > out->size)
        return SSC_KERNEL_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER;

    switch (state->signaturesCount) {
        case SSC_PREFERRED_EFFICIENCY_CHECK_SIGNATURES:
            if (state->efficiencyChecked ^ 0x1) {
                state->efficiencyChecked = 1;
                return SSC_KERNEL_ENCODE_STATE_INFO_EFFICIENCY_CHECK;
            }
            break;
        case SSC_PREFERRED_BLOCK_SIGNATURES:
            state->signaturesCount = 0;
            state->efficiencyChecked = 0;

            if (state->resetCycle)
                state->resetCycle--;
            else {
                CHAMELEON_NAME(ssc_dictionary_reset)(&state->dictionary);
                state->resetCycle = SSC_DICTIONARY_PREFERRED_RESET_CYCLE - 1;
            }

            return SSC_KERNEL_ENCODE_STATE_INFO_NEW_BLOCK;
        default:
            break;
    }
    state->signaturesCount++;

    state->shift = 0;
    state->signature = (ssc_hash_signature *) (out->pointer + out->position);
    *state->signature = 0;
    out->position += sizeof(ssc_hash_signature);

    return SSC_KERNEL_ENCODE_STATE_READY;
}

SSC_FORCE_INLINE SSC_KERNEL_ENCODE_STATE ssc_hash_encode_checkState(ssc_byte_buffer *restrict out, ssc_chameleon_encode_state *restrict state) {
    SSC_KERNEL_ENCODE_STATE returnState;

    switch (state->shift) {
        case 64:
            if ((returnState = ssc_hash_encode_prepareNewBlock(out, state, SSC_HASH_ENCODE_MINIMUM_OUTPUT_LOOKAHEAD))) {
                state->process = SSC_HASH_ENCODE_PROCESS_PREPARE_NEW_BLOCK;
                return returnState;
            }
            break;
        default:
            break;
    }

    return SSC_KERNEL_ENCODE_STATE_READY;
}

SSC_FORCE_INLINE void ssc_hash_encode_kernel(ssc_byte_buffer *restrict out, uint32_t *restrict hash, const uint32_t chunk, ssc_chameleon_encode_state *restrict state) {
    SSC_CHAMELEON_HASH_ALGORITHM(*hash, SSC_LITTLE_ENDIAN_32(chunk));
    ssc_dictionary_entry *found = &state->dictionary.entries[*hash];

    if (chunk ^ found->as_uint32_t) {
        found->as_uint32_t = chunk;
        *(uint32_t *) (out->pointer + out->position) = chunk;
        out->position += sizeof(uint32_t);
    } else {
        ssc_hash_encode_writeToSignature(state);
        *(uint16_t *) (out->pointer + out->position) = SSC_LITTLE_ENDIAN_16(*hash);
        out->position += sizeof(uint16_t);
    }

    state->shift++;
}

SSC_FORCE_INLINE void ssc_hash_encode_process_chunk(uint64_t *chunk, ssc_byte_buffer *restrict in, ssc_byte_buffer *restrict out, uint32_t *restrict hash, ssc_chameleon_encode_state *restrict state) {
    *chunk = *(uint64_t *) (in->pointer + in->position);
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    ssc_hash_encode_kernel(out, hash, (uint32_t) (*chunk & 0xFFFFFFFF), state);
#endif
    ssc_hash_encode_kernel(out, hash, (uint32_t) (*chunk >> 32), state);
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    ssc_hash_encode_kernel(out, hash, (uint32_t) (*chunk & 0xFFFFFFFF), state);
#endif
    in->position += sizeof(uint64_t);
}

SSC_FORCE_INLINE void ssc_hash_encode_process_span(uint64_t *chunk, ssc_byte_buffer *restrict in, ssc_byte_buffer *restrict out, uint32_t *restrict hash, ssc_chameleon_encode_state *restrict state) {
    ssc_hash_encode_process_chunk(chunk, in, out, hash, state);
    ssc_hash_encode_process_chunk(chunk, in, out, hash, state);
    ssc_hash_encode_process_chunk(chunk, in, out, hash, state);
    ssc_hash_encode_process_chunk(chunk, in, out, hash, state);
}

SSC_FORCE_INLINE ssc_bool ssc_hash_encode_attempt_copy(ssc_byte_buffer *restrict out, ssc_byte *restrict origin, const uint_fast32_t count) {
    if (out->position + count <= out->size) {
        memcpy(out->pointer + out->position, origin, count);
        out->position += count;
        return false;
    }
    return true;
}

SSC_FORCE_INLINE SSC_KERNEL_ENCODE_STATE CHAMELEON_NAME(ssc_chameleon_encode_init)(ssc_chameleon_encode_state *state) {
    state->signaturesCount = 0;
    state->efficiencyChecked = 0;
    CHAMELEON_NAME(ssc_dictionary_reset)(&state->dictionary);
    state->resetCycle = SSC_DICTIONARY_PREFERRED_RESET_CYCLE - 1;

    state->process = SSC_HASH_ENCODE_PROCESS_PREPARE_NEW_BLOCK;

    return SSC_KERNEL_ENCODE_STATE_READY;
}

SSC_FORCE_INLINE SSC_KERNEL_ENCODE_STATE CHAMELEON_NAME(ssc_chameleon_encode_process)(ssc_byte_buffer *restrict in, ssc_byte_buffer *restrict out, ssc_chameleon_encode_state *restrict state, const ssc_bool flush) {
    SSC_KERNEL_ENCODE_STATE returnState;
    uint32_t hash;
    uint_fast64_t remaining;
    uint64_t chunk;

    if (in->size == 0)
        goto exit;

    const uint_fast64_t limit = in->size & ~0x1F;

    switch (state->process) {
        case SSC_HASH_ENCODE_PROCESS_CHECK_STATE:
            if ((returnState = ssc_hash_encode_checkState(out, state)))
                return returnState;
            state->process = SSC_HASH_ENCODE_PROCESS_DATA;
            break;

        case SSC_HASH_ENCODE_PROCESS_PREPARE_NEW_BLOCK:
            if ((returnState = ssc_hash_encode_prepareNewBlock(out, state, SSC_HASH_ENCODE_MINIMUM_OUTPUT_LOOKAHEAD)))
                return returnState;
            state->process = SSC_HASH_ENCODE_PROCESS_DATA;
            break;

        case SSC_HASH_ENCODE_PROCESS_DATA:
            if (in->size - in->position < 4 * sizeof(uint64_t))
                goto finish;
            while (true) {
                ssc_hash_encode_process_span(&chunk, in, out, &hash, state);
                if (in->position == limit) {
                    if (flush) {
                        state->process = SSC_HASH_ENCODE_PROCESS_FINISH;
                        return SSC_KERNEL_ENCODE_STATE_READY;
                    } else {
                        state->process = SSC_HASH_ENCODE_PROCESS_CHECK_STATE;
                        return SSC_KERNEL_ENCODE_STATE_STALL_ON_INPUT_BUFFER;
                    }
                }

                if ((returnState = ssc_hash_encode_checkState(out, state)))
                    return returnState;
            }

        case SSC_HASH_ENCODE_PROCESS_FINISH:
            while (true) {
                while (state->shift ^ 64) {
                    if (in->size - in->position < sizeof(uint32_t))
                        goto finish;
                    else {
                        if (out->size - out->position < sizeof(uint32_t))
                            return SSC_KERNEL_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER;
                        ssc_hash_encode_kernel(out, &hash, *(uint32_t *) (in->pointer + in->position), state);
                        in->position += sizeof(uint32_t);
                    }
                }
                if (in->size - in->position < sizeof(uint32_t))
                    goto finish;
                else if ((returnState = ssc_hash_encode_prepareNewBlock(out, state, sizeof(ssc_hash_signature))))
                    return returnState;
            }
        finish:
            remaining = in->size - in->position;
            if (remaining > 0) {
                if (ssc_hash_encode_attempt_copy(out, in->pointer + in->position, (uint32_t) remaining))
                    return SSC_KERNEL_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER;
                in->position += remaining;
            }
        exit:
            state->process = SSC_HASH_ENCODE_PROCESS_PREPARE_NEW_BLOCK;
            return SSC_KERNEL_ENCODE_STATE_FINISHED;

        default:
            return SSC_KERNEL_ENCODE_STATE_ERROR;
    }

    return
            SSC_KERNEL_ENCODE_STATE_READY;
}

SSC_FORCE_INLINE SSC_KERNEL_ENCODE_STATE CHAMELEON_NAME(ssc_chameleon_encode_finish)(ssc_chameleon_encode_state *state) {
    return SSC_KERNEL_ENCODE_STATE_READY;
}

#endif