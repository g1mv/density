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
 * 07/12/13 15:49
 *
 * -----------------
 * Mandala algorithm
 * -----------------
 *
 * Author(s)
 * Guillaume Voirin
 * Piotr Tarsa
 *
 * Description
 * Very fast two level dictionary hash algorithm with predictions derived from Chameleon
 */

#include "kernel_mandala_decode.h"

DENSITY_FORCE_INLINE DENSITY_KERNEL_DECODE_STATE density_mandala_decode_check_signatures_count(density_mandala_decode_state *restrict state) {
    switch (state->signaturesCount) {
        case DENSITY_PREFERRED_EFFICIENCY_CHECK_SIGNATURES:
            if (state->efficiencyChecked ^ 0x1) {
                state->efficiencyChecked = 1;
                return DENSITY_KERNEL_DECODE_STATE_INFO_EFFICIENCY_CHECK;
            }
            break;
        case DENSITY_PREFERRED_BLOCK_SIGNATURES:
            state->signaturesCount = 0;
            state->efficiencyChecked = 0;

            if (state->resetCycle)
                state->resetCycle--;
            else {
                density_byte resetDictionaryCycleShift = state->parameters.as_bytes[0];
                if (resetDictionaryCycleShift) {
                    density_mandala_dictionary_reset(&state->dictionary);
                    state->resetCycle = (uint_fast64_t) (1 << resetDictionaryCycleShift) - 1;
                }
            }

            return DENSITY_KERNEL_DECODE_STATE_INFO_NEW_BLOCK;
        default:
            break;
    }
    return DENSITY_KERNEL_DECODE_STATE_READY;
}

DENSITY_FORCE_INLINE void density_mandala_decode_read_signature_fast(density_byte_buffer *restrict in, density_mandala_decode_state *restrict state) {
    state->signature = DENSITY_LITTLE_ENDIAN_64(*(density_mandala_signature *) (in->pointer + in->position));
    in->position += sizeof(density_mandala_signature);
    state->shift = 0;
    state->signaturesCount++;
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_DECODE_STATE density_mandala_decode_read_signature_safe(density_byte_buffer *restrict in, density_mandala_decode_state *restrict state) {
    if (state->signatureBytes) {
        memcpy(&state->partialSignature.as_bytes[state->signatureBytes], in->pointer + in->position, (uint32_t) (sizeof(density_mandala_signature) - state->signatureBytes));
        state->signature = DENSITY_LITTLE_ENDIAN_64(state->partialSignature.as_uint64_t);
        in->position += sizeof(density_mandala_signature) - state->signatureBytes;
        state->signatureBytes = 0;
        state->shift = 0;
        state->signaturesCount++;
    } else if (in->position + sizeof(density_mandala_signature) > in->size) {
        state->signatureBytes = in->size - in->position;
        memcpy(&state->partialSignature.as_bytes[0], in->pointer + in->position, (uint32_t) state->signatureBytes);
        in->position = in->size;
        return DENSITY_KERNEL_DECODE_STATE_STALL_ON_INPUT_BUFFER;
    } else
        density_mandala_decode_read_signature_fast(in, state);

    return DENSITY_KERNEL_DECODE_STATE_READY;
}

DENSITY_FORCE_INLINE void density_mandala_decode_read_compressed_chunk_fast(uint32_t *restrict hash, density_byte_buffer *restrict in) {
    *hash = *(uint16_t *) (in->pointer + in->position);
    in->position += sizeof(uint16_t);
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_DECODE_STATE density_mandala_decode_read_compressed_chunk_safe(uint32_t *restrict hash, density_byte_buffer *restrict in) {
    if (in->position + sizeof(uint16_t) > in->size)
        return DENSITY_KERNEL_DECODE_STATE_STALL_ON_INPUT_BUFFER;

    density_mandala_decode_read_compressed_chunk_fast(hash, in);

    return DENSITY_KERNEL_DECODE_STATE_READY;
}

DENSITY_FORCE_INLINE void density_mandala_decode_read_uncompressed_chunk_fast(uint32_t *restrict chunk, density_byte_buffer *restrict in) {
    *chunk = *(uint32_t *) (in->pointer + in->position);
    in->position += sizeof(uint32_t);
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_DECODE_STATE density_mandala_decode_read_uncompressed_chunk_safe(uint32_t *restrict chunk, density_byte_buffer *restrict in, density_mandala_decode_state *restrict state) {
    if (state->uncompressedChunkBytes) {
        memcpy(&state->partialUncompressedChunk.as_bytes[state->uncompressedChunkBytes], in->pointer + in->position, (uint32_t) (sizeof(uint32_t) - state->uncompressedChunkBytes));
        *chunk = state->partialUncompressedChunk.as_uint32_t;
        in->position += sizeof(uint32_t) - state->uncompressedChunkBytes;
        state->uncompressedChunkBytes = 0;
    } else if (in->position + sizeof(uint32_t) > in->size) {
        state->uncompressedChunkBytes = in->size - in->position;
        memcpy(&state->partialUncompressedChunk.as_bytes[0], in->pointer + in->position, (uint32_t) state->uncompressedChunkBytes);
        in->position = in->size;
        return DENSITY_KERNEL_DECODE_STATE_STALL_ON_INPUT_BUFFER;
    } else
        density_mandala_decode_read_uncompressed_chunk_fast(chunk, in);

    return DENSITY_KERNEL_DECODE_STATE_READY;
}

DENSITY_FORCE_INLINE void density_mandala_decode_predicted_chunk(uint32_t *restrict hash, density_byte_buffer *restrict out, density_mandala_decode_state *restrict state) {
    uint32_t chunk = (&state->dictionary.prediction_entries[state->lastHash])->next_chunk_prediction;
    DENSITY_MANDALA_HASH_ALGORITHM(*hash, DENSITY_LITTLE_ENDIAN_32(chunk));

    *(uint32_t *) (out->pointer + out->position) = chunk;
    out->position += sizeof(uint32_t);
}

DENSITY_FORCE_INLINE void density_mandala_write_decompressed_chunk(const uint32_t *restrict chunk, density_byte_buffer *restrict out, density_mandala_decode_state *restrict state) {
    *(uint32_t *) (out->pointer + out->position) = *chunk;
    out->position += sizeof(uint32_t);

    (&state->dictionary.prediction_entries[state->lastHash])->next_chunk_prediction = *chunk;
}

DENSITY_FORCE_INLINE void density_mandala_decode_compressed_chunk_a(const uint32_t *restrict hash, density_byte_buffer *restrict out, density_mandala_decode_state *restrict state) {
    uint32_t chunk = (&state->dictionary.entries[DENSITY_LITTLE_ENDIAN_16(*hash)])->chunk_a;

    density_mandala_write_decompressed_chunk(&chunk, out, state);
}

DENSITY_FORCE_INLINE void density_mandala_decode_compressed_chunk_b(const uint32_t *restrict hash, density_byte_buffer *restrict out, density_mandala_decode_state *restrict state) {
    density_mandala_dictionary_entry *entry = &state->dictionary.entries[DENSITY_LITTLE_ENDIAN_16(*hash)];
    uint32_t swapped_chunk = entry->chunk_b;

    entry->chunk_b = entry->chunk_a;
    entry->chunk_a = swapped_chunk;

    density_mandala_write_decompressed_chunk(&swapped_chunk, out, state);
}

DENSITY_FORCE_INLINE void density_mandala_decode_uncompressed_chunk(uint32_t *restrict hash, const uint32_t *restrict chunk, density_byte_buffer *restrict out, density_mandala_decode_state *restrict state) {
    density_mandala_dictionary_entry *entry;

    DENSITY_MANDALA_HASH_ALGORITHM(*hash, DENSITY_LITTLE_ENDIAN_32(*chunk));
    entry = &state->dictionary.entries[*hash];
    entry->chunk_b = entry->chunk_a;
    entry->chunk_a = *chunk;

    density_mandala_write_decompressed_chunk(chunk, out, state);
}

DENSITY_FORCE_INLINE void density_mandala_decode_kernel_fast(density_byte_buffer *restrict in, density_byte_buffer *restrict out, const DENSITY_MANDALA_SIGNATURE_FLAG mode, density_mandala_decode_state *restrict state) {
    uint32_t hash = 0;
    uint32_t chunk;

    switch (mode) {
        case DENSITY_MANDALA_SIGNATURE_FLAG_PREDICTED:
            density_mandala_decode_predicted_chunk(&hash, out, state);
            break;
        case DENSITY_MANDALA_SIGNATURE_FLAG_MAP_A:
            density_mandala_decode_read_compressed_chunk_fast(&hash, in);
            density_mandala_decode_compressed_chunk_a(&hash, out, state);
            break;
        case DENSITY_MANDALA_SIGNATURE_FLAG_MAP_B:
            density_mandala_decode_read_compressed_chunk_fast(&hash, in);
            density_mandala_decode_compressed_chunk_b(&hash, out, state);
            break;
        case DENSITY_MANDALA_SIGNATURE_FLAG_CHUNK:
            density_mandala_decode_read_uncompressed_chunk_fast(&chunk, in);
            density_mandala_decode_uncompressed_chunk(&hash, &chunk, out, state);
            break;
    }

    state->lastHash = hash;
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_DECODE_STATE density_mandala_decode_kernel_safe(density_byte_buffer *restrict in, density_byte_buffer *restrict out, density_mandala_decode_state *restrict state, const DENSITY_MANDALA_SIGNATURE_FLAG mode) {
    DENSITY_KERNEL_DECODE_STATE returnState;

    if (out->position + sizeof(uint32_t) > out->size)
        return DENSITY_KERNEL_DECODE_STATE_STALL_ON_OUTPUT_BUFFER;

    uint32_t hash = 0;
    uint32_t chunk;
    switch (mode) {
        case DENSITY_MANDALA_SIGNATURE_FLAG_PREDICTED:
            density_mandala_decode_predicted_chunk(&hash, out, state);
            break;
        case DENSITY_MANDALA_SIGNATURE_FLAG_MAP_A:
            if ((returnState = density_mandala_decode_read_compressed_chunk_safe(&hash, in)))
                return returnState;
            density_mandala_decode_compressed_chunk_a(&hash, out, state);
            break;
        case DENSITY_MANDALA_SIGNATURE_FLAG_MAP_B:
            if ((returnState = density_mandala_decode_read_compressed_chunk_safe(&hash, in)))
                return returnState;
            density_mandala_decode_compressed_chunk_b(&hash, out, state);
            break;
        case DENSITY_MANDALA_SIGNATURE_FLAG_CHUNK:
            if ((returnState = density_mandala_decode_read_uncompressed_chunk_safe(&chunk, in, state)))
                return returnState;
            density_mandala_decode_uncompressed_chunk(&hash, &chunk, out, state);
            break;
    }

    state->lastHash = hash;

    return DENSITY_KERNEL_DECODE_STATE_READY;
}

DENSITY_FORCE_INLINE const DENSITY_MANDALA_SIGNATURE_FLAG density_mandala_decode_get_signature_flag(density_mandala_decode_state *state) {
    return (DENSITY_MANDALA_SIGNATURE_FLAG const) ((state->signature >> state->shift) & 0x3);
}

DENSITY_FORCE_INLINE void density_mandala_decode_process_data_fast(density_byte_buffer *restrict in, density_byte_buffer *restrict out, density_mandala_decode_state *restrict state) {
    while (state->shift ^ 64) {
        density_mandala_decode_kernel_fast(in, out, density_mandala_decode_get_signature_flag(state), state);
        state->shift += 2;
    }
}

DENSITY_FORCE_INLINE density_bool density_mandala_decode_attempt_copy(density_byte_buffer *restrict out, density_byte *restrict origin, const uint_fast32_t count) {
    if (out->position + count <= out->size) {
        memcpy(out->pointer + out->position, origin, count);
        out->position += count;
        return false;
    }
    return true;
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_DECODE_STATE density_mandala_decode_init(density_mandala_decode_state *state, const density_main_header_parameters parameters, const uint_fast32_t endDataOverhead) {
    state->signaturesCount = 0;
    state->efficiencyChecked = 0;
    density_mandala_dictionary_reset(&state->dictionary);

    state->parameters = parameters;
    density_byte resetDictionaryCycleShift = state->parameters.as_bytes[0];
    if (resetDictionaryCycleShift)
        state->resetCycle = (uint_fast64_t) (1 << resetDictionaryCycleShift) - 1;

    state->endDataOverhead = endDataOverhead;

    state->signatureBytes = 0;
    state->uncompressedChunkBytes = 0;

    state->process = DENSITY_MANDALA_DECODE_PROCESS_SIGNATURES_AND_DATA_FAST;

    return DENSITY_KERNEL_DECODE_STATE_READY;
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_DECODE_STATE density_mandala_decode_process(density_byte_buffer *restrict in, density_byte_buffer *restrict out, density_mandala_decode_state *restrict state, const density_bool flush) {
    DENSITY_KERNEL_DECODE_STATE returnState;
    uint_fast64_t remaining;
    uint_fast64_t limitIn = 0;
    uint_fast64_t limitOut = 0;

    if (in->size > DENSITY_MANDALA_DECODE_MINIMUM_INPUT_LOOKAHEAD + state->endDataOverhead)
        limitIn = in->size - DENSITY_MANDALA_DECODE_MINIMUM_INPUT_LOOKAHEAD - state->endDataOverhead;
    if (out->size > DENSITY_MANDALA_DECODE_MINIMUM_OUTPUT_LOOKAHEAD)
        limitOut = out->size - DENSITY_MANDALA_DECODE_MINIMUM_OUTPUT_LOOKAHEAD;

    switch (state->process) {
        case DENSITY_MANDALA_DECODE_PROCESS_SIGNATURES_AND_DATA_FAST:
            while (in->position < limitIn && out->position < limitOut) {
                if ((returnState = density_mandala_decode_check_signatures_count(state)))
                    return returnState;

                density_mandala_decode_read_signature_fast(in, state);
                density_mandala_decode_process_data_fast(in, out, state);
            }
            state->process = DENSITY_MANDALA_DECODE_PROCESS_SIGNATURE_SAFE;
            break;

        case DENSITY_MANDALA_DECODE_PROCESS_SIGNATURE_SAFE:
            if (flush && (in->size - in->position < sizeof(density_mandala_signature) + state->endDataOverhead)) {
                state->process = DENSITY_MANDALA_DECODE_PROCESS_FINISH;
                return DENSITY_KERNEL_DECODE_STATE_READY;
            }

            if ((returnState = density_mandala_decode_check_signatures_count(state)))
                return returnState;

            if ((returnState = density_mandala_decode_read_signature_safe(in, state)))
                return returnState;

            if (in->position < limitIn && out->position < limitOut) {
                state->process = DENSITY_MANDALA_DECODE_PROCESS_DATA_FAST;
                return DENSITY_KERNEL_DECODE_STATE_READY;
            }
            state->process = DENSITY_MANDALA_DECODE_PROCESS_DATA_SAFE;
            break;

        case DENSITY_MANDALA_DECODE_PROCESS_DATA_SAFE:
            while (state->shift ^ 64) {
                DENSITY_MANDALA_SIGNATURE_FLAG flag = density_mandala_decode_get_signature_flag(state);
                size_t overhead = 0;
                switch (flag) {
                    case DENSITY_MANDALA_SIGNATURE_FLAG_MAP_A:
                    case DENSITY_MANDALA_SIGNATURE_FLAG_MAP_B:
                        overhead = sizeof(uint16_t);
                        break;
                    case  DENSITY_MANDALA_SIGNATURE_FLAG_CHUNK:
                        overhead = sizeof(uint32_t);
                        break;
                    default:
                        break;
                }
                if (flush && (in->size - in->position < overhead + state->endDataOverhead)) {
                    state->process = DENSITY_MANDALA_DECODE_PROCESS_FINISH;
                    return DENSITY_KERNEL_DECODE_STATE_READY;
                }
                if ((returnState = density_mandala_decode_kernel_safe(in, out, state, flag)))
                    return returnState;
                state->shift += 2;
                if (in->position < limitIn && out->position < limitOut) {
                    state->process = DENSITY_MANDALA_DECODE_PROCESS_DATA_FAST;
                    return DENSITY_KERNEL_DECODE_STATE_READY;
                }
            }
            state->process = DENSITY_MANDALA_DECODE_PROCESS_SIGNATURES_AND_DATA_FAST;
            break;

        case DENSITY_MANDALA_DECODE_PROCESS_DATA_FAST:
            density_mandala_decode_process_data_fast(in, out, state);
            state->process = DENSITY_MANDALA_DECODE_PROCESS_SIGNATURES_AND_DATA_FAST;
            break;

        case DENSITY_MANDALA_DECODE_PROCESS_FINISH:
            if (state->uncompressedChunkBytes) {
                if (density_mandala_decode_attempt_copy(out, state->partialUncompressedChunk.as_bytes, (uint32_t) state->uncompressedChunkBytes))
                    return DENSITY_KERNEL_DECODE_STATE_STALL_ON_OUTPUT_BUFFER;
                state->uncompressedChunkBytes = 0;
            }
            remaining = in->size - in->position;
            if (remaining > state->endDataOverhead) {
                if (density_mandala_decode_attempt_copy(out, in->pointer + in->position, (uint32_t) (remaining - state->endDataOverhead)))
                    return DENSITY_KERNEL_DECODE_STATE_STALL_ON_OUTPUT_BUFFER;
                in->position += (remaining - state->endDataOverhead);
            }
            state->process = DENSITY_MANDALA_DECODE_PROCESS_SIGNATURES_AND_DATA_FAST;
            return DENSITY_KERNEL_DECODE_STATE_FINISHED;

        default:
            return DENSITY_KERNEL_DECODE_STATE_ERROR;
    }

    return DENSITY_KERNEL_DECODE_STATE_READY;
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_DECODE_STATE density_mandala_decode_finish(density_mandala_decode_state *state) {
    return DENSITY_KERNEL_DECODE_STATE_READY;
}
