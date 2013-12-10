/*
 * Centaurean Density
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
 * 06/12/13 20:28
 *
 * -----------------
 * Mandala algorithm
 * -----------------
 *
 * Author(s)
 * Guillaume Voirin (https://github.com/gpnuma)
 * Piotr Tarsa (https://github.com/tarsa)
 *
 * Description
 * Very fast two level dictionary hash algorithm derived from Chameleon, with predictions lookup
 */

#include "kernel_mandala_encode.h"

DENSITY_FORCE_INLINE void density_mandala_encode_write_to_signature(density_mandala_encode_state *state, uint_fast8_t flag) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    *(state->signature) |= ((uint64_t) flag) << state->shift;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    *(state->signature) |= ((uint64_t) flag) << ((56 - (state->shift & ~0x7)) + (state->shift & 0x7));
#endif
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE density_mandala_encode_prepare_new_block(density_byte_buffer *restrict out, density_mandala_encode_state *restrict state, const uint_fast32_t minimumLookahead) {
    if (out->position + minimumLookahead > out->size)
        return DENSITY_KERNEL_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER;

    switch (state->signaturesCount) {
        case DENSITY_MANDALA_PREFERRED_EFFICIENCY_CHECK_SIGNATURES:
            if (state->efficiencyChecked ^ 0x1) {
                state->efficiencyChecked = 1;
                return DENSITY_KERNEL_ENCODE_STATE_INFO_EFFICIENCY_CHECK;
            }
            break;
        case DENSITY_MANDALA_PREFERRED_BLOCK_SIGNATURES:
            state->signaturesCount = 0;
            state->efficiencyChecked = 0;

#if DENSITY_ENABLE_PARALLELIZABLE_DECOMPRESSIBLE_OUTPUT == DENSITY_YES
            if (state->resetCycle)
                state->resetCycle--;
            else {
                density_mandala_dictionary_reset(&state->dictionary);
                state->resetCycle = DENSITY_DICTIONARY_PREFERRED_RESET_CYCLE - 1;
            }
#endif

            return DENSITY_KERNEL_ENCODE_STATE_INFO_NEW_BLOCK;
        default:
            break;
    }
    state->signaturesCount++;

    state->shift = 0;
    state->signature = (density_mandala_signature *) (out->pointer + out->position);
    *state->signature = 0;
    out->position += sizeof(density_mandala_signature);

    return DENSITY_KERNEL_ENCODE_STATE_READY;
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE density_mandala_encode_check_state(density_byte_buffer *restrict out, density_mandala_encode_state *restrict state) {
    DENSITY_KERNEL_ENCODE_STATE returnState;

    switch (state->shift) {
        case 64:
            if ((returnState = density_mandala_encode_prepare_new_block(out, state, DENSITY_MANDALA_ENCODE_MINIMUM_OUTPUT_LOOKAHEAD))) {
                state->process = DENSITY_MANDALA_ENCODE_PROCESS_PREPARE_NEW_BLOCK;
                return returnState;
            }
            break;
        default:
            break;
    }

    return DENSITY_KERNEL_ENCODE_STATE_READY;
}

DENSITY_FORCE_INLINE void density_mandala_encode_kernel(density_byte_buffer *restrict out, uint32_t *restrict hash, const uint32_t chunk, density_mandala_encode_state *restrict state) {
    DENSITY_MANDALA_HASH_ALGORITHM(*hash, DENSITY_LITTLE_ENDIAN_32(chunk));
    uint32_t *predictedChunk = &(state->dictionary.prediction_entries[state->lastHash].next_chunk_prediction);

    if (*predictedChunk ^ chunk) {
        density_mandala_dictionary_entry *found = &state->dictionary.entries[*hash];
        uint32_t *found_a = &found->chunk_a;
        if (*found_a ^ chunk) {
            uint32_t *found_b = &found->chunk_b;
            if (*found_b ^ chunk) {
                density_mandala_encode_write_to_signature(state, DENSITY_MANDALA_SIGNATURE_FLAG_CHUNK);
                *(uint32_t *) (out->pointer + out->position) = chunk;
                out->position += sizeof(uint32_t);
            } else {
                density_mandala_encode_write_to_signature(state, DENSITY_MANDALA_SIGNATURE_FLAG_MAP_B);
                *(uint16_t *) (out->pointer + out->position) = DENSITY_LITTLE_ENDIAN_16(*hash);
                out->position += sizeof(uint16_t);
            }
            *found_b = *found_a;
            *found_a = chunk;
        } else {
            //density_mandala_encode_write_to_signature(state, DENSITY_MANDALA_SIGNATURE_FLAG_MAP_A);
            *(uint16_t *) (out->pointer + out->position) = DENSITY_LITTLE_ENDIAN_16(*hash);
            out->position += sizeof(uint16_t);
        }
        *predictedChunk = chunk;
    } else {
        density_mandala_encode_write_to_signature(state, DENSITY_MANDALA_SIGNATURE_FLAG_PREDICTED);
    }
    state->lastHash = (uint16_t) (*hash & 0xFFFF);

    state->shift += 2;
}

DENSITY_FORCE_INLINE void density_mandala_encode_process_chunk(uint64_t *chunk, density_byte_buffer *restrict in, density_byte_buffer *restrict out, uint32_t *restrict hash, density_mandala_encode_state *restrict state) {
    *chunk = *(uint64_t *) (in->pointer + in->position);
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    density_mandala_encode_kernel(out, hash, (uint32_t) (*chunk & 0xFFFFFFFF), state);
#endif
    density_mandala_encode_kernel(out, hash, (uint32_t) (*chunk >> 32), state);
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    density_mandala_encode_kernel(out, hash, (uint32_t) (*chunk & 0xFFFFFFFF), state);
#endif
    in->position += sizeof(uint64_t);
}

DENSITY_FORCE_INLINE void density_mandala_encode_process_span(uint64_t *chunk, density_byte_buffer *restrict in, density_byte_buffer *restrict out, uint32_t *restrict hash, density_mandala_encode_state *restrict state) {
    density_mandala_encode_process_chunk(chunk, in, out, hash, state);
    density_mandala_encode_process_chunk(chunk, in, out, hash, state);
    density_mandala_encode_process_chunk(chunk, in, out, hash, state);
    density_mandala_encode_process_chunk(chunk, in, out, hash, state);
}

DENSITY_FORCE_INLINE density_bool density_mandala_encode_attempt_copy(density_byte_buffer *restrict out, density_byte *restrict origin, const uint_fast32_t count) {
    if (out->position + count <= out->size) {
        memcpy(out->pointer + out->position, origin, count);
        out->position += count;
        return false;
    }
    return true;
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE density_mandala_encode_init(density_mandala_encode_state *state) {
    state->signaturesCount = 0;
    state->efficiencyChecked = 0;
    density_mandala_dictionary_reset(&state->dictionary);

#if DENSITY_ENABLE_PARALLELIZABLE_DECOMPRESSIBLE_OUTPUT == DENSITY_YES
    state->resetCycle = DENSITY_DICTIONARY_PREFERRED_RESET_CYCLE - 1;
#endif

    state->process = DENSITY_MANDALA_ENCODE_PROCESS_PREPARE_NEW_BLOCK;

    return DENSITY_KERNEL_ENCODE_STATE_READY;
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE density_mandala_encode_process(density_byte_buffer *restrict in, density_byte_buffer *restrict out, density_mandala_encode_state *restrict state, const density_bool flush) {
    DENSITY_KERNEL_ENCODE_STATE returnState;
    uint32_t hash;
    uint_fast64_t remaining;
    uint64_t chunk;

    if (in->size == 0)
        goto exit;

    const uint_fast64_t limit = in->size & ~0x1F;

    switch (state->process) {
        case DENSITY_MANDALA_ENCODE_PROCESS_CHECK_STATE:
            if ((returnState = density_mandala_encode_check_state(out, state)))
                return returnState;
            state->process = DENSITY_MANDALA_ENCODE_PROCESS_DATA;
            break;

        case DENSITY_MANDALA_ENCODE_PROCESS_PREPARE_NEW_BLOCK:
            if ((returnState = density_mandala_encode_prepare_new_block(out, state, DENSITY_MANDALA_ENCODE_MINIMUM_OUTPUT_LOOKAHEAD)))
                return returnState;
            state->process = DENSITY_MANDALA_ENCODE_PROCESS_DATA;
            break;

        case DENSITY_MANDALA_ENCODE_PROCESS_DATA:
            if (in->size - in->position < 4 * sizeof(uint64_t))
                goto finish;
            while (true) {
                density_mandala_encode_process_span(&chunk, in, out, &hash, state);
                if (in->position == limit) {
                    if (flush) {
                        state->process = DENSITY_MANDALA_ENCODE_PROCESS_FINISH;
                        return DENSITY_KERNEL_ENCODE_STATE_READY;
                    } else {
                        state->process = DENSITY_MANDALA_ENCODE_PROCESS_CHECK_STATE;
                        return DENSITY_KERNEL_ENCODE_STATE_STALL_ON_INPUT_BUFFER;
                    }
                }

                if ((returnState = density_mandala_encode_check_state(out, state)))
                    return returnState;
            }

        case DENSITY_MANDALA_ENCODE_PROCESS_FINISH:
            while (true) {
                while (state->shift ^ 64) {
                    if (in->size - in->position < sizeof(uint32_t))
                        goto finish;
                    else {
                        if (out->size - out->position < sizeof(uint32_t))
                            return DENSITY_KERNEL_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER;
                        density_mandala_encode_kernel(out, &hash, *(uint32_t *) (in->pointer + in->position), state);
                        in->position += sizeof(uint32_t);
                    }
                }
                if (in->size - in->position < sizeof(uint32_t))
                    goto finish;
                else if ((returnState = density_mandala_encode_prepare_new_block(out, state, sizeof(density_mandala_signature))))
                    return returnState;
            }
        finish:
            remaining = in->size - in->position;
            if (remaining > 0) {
                if (state->shift ^ 64)
                    density_mandala_encode_write_to_signature(state, DENSITY_MANDALA_SIGNATURE_FLAG_CHUNK);
                if (density_mandala_encode_attempt_copy(out, in->pointer + in->position, (uint32_t) remaining))
                    return DENSITY_KERNEL_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER;
                in->position += remaining;
            }
        exit:
            state->process = DENSITY_MANDALA_ENCODE_PROCESS_PREPARE_NEW_BLOCK;
            return DENSITY_KERNEL_ENCODE_STATE_FINISHED;

        default:
            return DENSITY_KERNEL_ENCODE_STATE_ERROR;
    }

    return DENSITY_KERNEL_ENCODE_STATE_READY;
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE density_mandala_encode_finish(density_mandala_encode_state *state) {
    return DENSITY_KERNEL_ENCODE_STATE_READY;
}