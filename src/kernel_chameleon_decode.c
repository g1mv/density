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
 * 24/10/13 12:28
 */

#ifdef SSC_CHAMELEON_CODE_INCLUSION

#include "kernel_chameleon_decode.h"

SSC_FORCE_INLINE SSC_KERNEL_DECODE_STATE ssc_hash_decode_checkSignaturesCount(ssc_chameleon_decode_state *restrict state) {
    switch (state->signaturesCount) {
        case SSC_PREFERRED_EFFICIENCY_CHECK_SIGNATURES:
            if (state->efficiencyChecked ^ 0x1) {
                state->efficiencyChecked = 1;
                return SSC_KERNEL_DECODE_STATE_INFO_EFFICIENCY_CHECK;
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

            return SSC_KERNEL_DECODE_STATE_INFO_NEW_BLOCK;
        default:
            break;
    }
    return SSC_KERNEL_DECODE_STATE_READY;
}

SSC_FORCE_INLINE void ssc_hash_decode_read_signature_fast(ssc_byte_buffer *restrict in, ssc_chameleon_decode_state *restrict state) {
    state->signature = SSC_LITTLE_ENDIAN_64(*(ssc_hash_signature *) (in->pointer + in->position));
    in->position += sizeof(ssc_hash_signature);
    state->shift = 0;
    state->signaturesCount++;
}

SSC_FORCE_INLINE SSC_KERNEL_DECODE_STATE ssc_hash_decode_read_signature_safe(ssc_byte_buffer *restrict in, ssc_chameleon_decode_state *restrict state) {
    if (state->signatureBytes) {
        memcpy(&state->partialSignature.as_bytes[state->signatureBytes], in->pointer + in->position, (uint32_t) (sizeof(ssc_hash_signature) - state->signatureBytes));
        state->signature = SSC_LITTLE_ENDIAN_64(state->partialSignature.as_uint64_t);
        in->position += sizeof(ssc_hash_signature) - state->signatureBytes;
        state->signatureBytes = 0;
        state->shift = 0;
        state->signaturesCount++;
    } else if (in->position + sizeof(ssc_hash_signature) > in->size) {
        state->signatureBytes = in->size - in->position;
        memcpy(&state->partialSignature.as_bytes[0], in->pointer + in->position, (uint32_t) state->signatureBytes);
        in->position = in->size;
        return SSC_KERNEL_DECODE_STATE_STALL_ON_INPUT_BUFFER;
    } else
        ssc_hash_decode_read_signature_fast(in, state);

    return SSC_KERNEL_DECODE_STATE_READY;
}

SSC_FORCE_INLINE void ssc_hash_decode_read_compressed_chunk_fast(uint16_t *chunk, ssc_byte_buffer *restrict in) {
    *chunk = *(uint16_t *) (in->pointer + in->position);
    in->position += sizeof(uint16_t);
}

SSC_FORCE_INLINE SSC_KERNEL_DECODE_STATE ssc_hash_decode_read_compressed_chunk_safe(uint16_t *restrict chunk, ssc_byte_buffer *restrict in) {
    if (in->position + sizeof(uint16_t) > in->size)
        return SSC_KERNEL_DECODE_STATE_STALL_ON_INPUT_BUFFER;

    ssc_hash_decode_read_compressed_chunk_fast(chunk, in);

    return SSC_KERNEL_DECODE_STATE_READY;
}

SSC_FORCE_INLINE void ssc_hash_decode_read_uncompressed_chunk_fast(uint32_t *chunk, ssc_byte_buffer *restrict in) {
    *chunk = *(uint32_t *) (in->pointer + in->position);
    in->position += sizeof(uint32_t);
}

SSC_FORCE_INLINE SSC_KERNEL_DECODE_STATE ssc_hash_decode_read_uncompressed_chunk_safe(uint32_t *restrict chunk, ssc_byte_buffer *restrict in, ssc_chameleon_decode_state *restrict state) {
    if (state->uncompressedChunkBytes) {
        memcpy(&state->partialUncompressedChunk.as_bytes[state->uncompressedChunkBytes], in->pointer + in->position, (uint32_t) (sizeof(uint32_t) - state->uncompressedChunkBytes));
        *chunk = state->partialUncompressedChunk.as_uint32_t;
        in->position += sizeof(uint32_t) - state->uncompressedChunkBytes;
        state->uncompressedChunkBytes = 0;
    } else if (in->position + sizeof(uint32_t) > in->size) {
        state->uncompressedChunkBytes = in->size - in->position;
        memcpy(&state->partialUncompressedChunk.as_bytes[0], in->pointer + in->position, (uint32_t) state->uncompressedChunkBytes);
        in->position = in->size;
        return SSC_KERNEL_DECODE_STATE_STALL_ON_INPUT_BUFFER;
    } else
        ssc_hash_decode_read_uncompressed_chunk_fast(chunk, in);

    return SSC_KERNEL_DECODE_STATE_READY;
}

SSC_FORCE_INLINE void ssc_hash_decode_compressed_chunk(const uint16_t *chunk, ssc_byte_buffer *restrict out, ssc_chameleon_decode_state *restrict state) {
    *(uint32_t *) (out->pointer + out->position) = (&state->dictionary.entries[SSC_LITTLE_ENDIAN_16(*chunk)])->as_uint32_t;
    out->position += sizeof(uint32_t);
}

SSC_FORCE_INLINE void ssc_hash_decode_uncompressed_chunk(const uint32_t *chunk, ssc_byte_buffer *restrict out, ssc_chameleon_decode_state *restrict state) {
    uint32_t hash;
    SSC_CHAMELEON_HASH_ALGORITHM(hash, SSC_LITTLE_ENDIAN_32(*chunk));
    (&state->dictionary.entries[hash])->as_uint32_t = *chunk;
    *(uint32_t *) (out->pointer + out->position) = *chunk;
    out->position += sizeof(uint32_t);
}

SSC_FORCE_INLINE void ssc_hash_decode_kernel_fast(ssc_byte_buffer *restrict in, ssc_byte_buffer *restrict out, const ssc_bool compressed, ssc_chameleon_decode_state *restrict state) {
    if (compressed) {
        uint16_t chunk;
        ssc_hash_decode_read_compressed_chunk_fast(&chunk, in);
        ssc_hash_decode_compressed_chunk(&chunk, out, state);
    } else {
        uint32_t chunk;
        ssc_hash_decode_read_uncompressed_chunk_fast(&chunk, in);
        ssc_hash_decode_uncompressed_chunk(&chunk, out, state);
    }
}

SSC_FORCE_INLINE SSC_KERNEL_DECODE_STATE ssc_hash_decode_kernel_safe(ssc_byte_buffer *restrict in, ssc_byte_buffer *restrict out, ssc_chameleon_decode_state *restrict state, const ssc_bool compressed) {
    SSC_KERNEL_DECODE_STATE returnState;

    if (out->position + sizeof(uint32_t) > out->size)
        return SSC_KERNEL_DECODE_STATE_STALL_ON_OUTPUT_BUFFER;

    if (compressed) {
        uint16_t chunk;
        if ((returnState = ssc_hash_decode_read_compressed_chunk_safe(&chunk, in)))
            return returnState;
        ssc_hash_decode_compressed_chunk(&chunk, out, state);
    } else {
        uint32_t chunk;
        if ((returnState = ssc_hash_decode_read_uncompressed_chunk_safe(&chunk, in, state)))
            return returnState;
        ssc_hash_decode_uncompressed_chunk(&chunk, out, state);
    }

    return SSC_KERNEL_DECODE_STATE_READY;
}

SSC_FORCE_INLINE const bool ssc_hash_decode_test_compressed(ssc_chameleon_decode_state *state) {
    return (ssc_bool const) ((state->signature >> state->shift) & 0x1);
}

SSC_FORCE_INLINE void ssc_hash_decode_process_data_fast(ssc_byte_buffer *restrict in, ssc_byte_buffer *restrict out, ssc_chameleon_decode_state *restrict state) {
    while (state->shift ^ 64) {
        ssc_hash_decode_kernel_fast(in, out, ssc_hash_decode_test_compressed(state), state);
        state->shift++;
    }
}

SSC_FORCE_INLINE ssc_bool ssc_hash_decode_attempt_copy(ssc_byte_buffer *restrict out, ssc_byte *restrict origin, const uint_fast32_t count) {
    if (out->position + count <= out->size) {
        memcpy(out->pointer + out->position, origin, count);
        out->position += count;
        return false;
    }
    return true;
}

SSC_FORCE_INLINE SSC_KERNEL_DECODE_STATE CHAMELEON_NAME(ssc_chameleon_decode_init)(ssc_chameleon_decode_state *state, const uint_fast32_t endDataOverhead) {
    state->signaturesCount = 0;
    state->efficiencyChecked = 0;
    CHAMELEON_NAME(ssc_dictionary_reset)(&state->dictionary);
    state->resetCycle = SSC_DICTIONARY_PREFERRED_RESET_CYCLE - 1;

    state->endDataOverhead = endDataOverhead;

    state->signatureBytes = 0;
    state->uncompressedChunkBytes = 0;

    state->process = SSC_CHAMELEON_DECODE_PROCESS_SIGNATURES_AND_DATA_FAST;

    return SSC_KERNEL_DECODE_STATE_READY;
}

SSC_FORCE_INLINE SSC_KERNEL_DECODE_STATE CHAMELEON_NAME(ssc_chameleon_decode_process)(ssc_byte_buffer *restrict in, ssc_byte_buffer *restrict out, ssc_chameleon_decode_state *restrict state, const ssc_bool flush) {
    SSC_KERNEL_DECODE_STATE returnState;
    uint_fast64_t remaining;
    uint_fast64_t limitIn = 0;
    uint_fast64_t limitOut = 0;

    if (in->size > SSC_HASH_DECODE_MINIMUM_INPUT_LOOKAHEAD + state->endDataOverhead)
        limitIn = in->size - SSC_HASH_DECODE_MINIMUM_INPUT_LOOKAHEAD - state->endDataOverhead;
    if (out->size > SSC_HASH_DECODE_MINIMUM_OUTPUT_LOOKAHEAD)
        limitOut = out->size - SSC_HASH_DECODE_MINIMUM_OUTPUT_LOOKAHEAD;

    switch (state->process) {
        case SSC_CHAMELEON_DECODE_PROCESS_SIGNATURES_AND_DATA_FAST:
            while (in->position < limitIn && out->position < limitOut) {
                if ((returnState = ssc_hash_decode_checkSignaturesCount(state)))
                    return returnState;

                ssc_hash_decode_read_signature_fast(in, state);
                ssc_hash_decode_process_data_fast(in, out, state);
            }
            state->process = SSC_CHAMELEON_DECODE_PROCESS_SIGNATURE_SAFE;
            break;

        case SSC_CHAMELEON_DECODE_PROCESS_SIGNATURE_SAFE:
            if (flush && (in->size - in->position < sizeof(ssc_hash_signature) + sizeof(uint16_t) + state->endDataOverhead)) {
                state->process = SSC_CHAMELEON_DECODE_PROCESS_FINISH;
                return SSC_KERNEL_DECODE_STATE_READY;
            }

            if ((returnState = ssc_hash_decode_checkSignaturesCount(state)))
                return returnState;

            if ((returnState = ssc_hash_decode_read_signature_safe(in, state)))
                return returnState;

            if (in->position < limitIn && out->position < limitOut) {
                state->process = SSC_CHAMELEON_DECODE_PROCESS_DATA_FAST;
                return SSC_KERNEL_DECODE_STATE_READY;
            }
            state->process = SSC_CHAMELEON_DECODE_PROCESS_DATA_SAFE;
            break;

        case SSC_CHAMELEON_DECODE_PROCESS_DATA_SAFE:
            while (state->shift ^ 64) {
                if (flush && (in->size - in->position < sizeof(uint16_t) + state->endDataOverhead + (ssc_hash_decode_test_compressed(state) ? 0 : 2))) {
                    state->process = SSC_CHAMELEON_DECODE_PROCESS_FINISH;
                    return SSC_KERNEL_DECODE_STATE_READY;
                }
                if ((returnState = ssc_hash_decode_kernel_safe(in, out, state, ssc_hash_decode_test_compressed(state))))
                    return returnState;
                state->shift++;
                if (in->position < limitIn && out->position < limitOut) {
                    state->process = SSC_CHAMELEON_DECODE_PROCESS_DATA_FAST;
                    return SSC_KERNEL_DECODE_STATE_READY;
                }
            }
            state->process = SSC_CHAMELEON_DECODE_PROCESS_SIGNATURES_AND_DATA_FAST;
            break;

        case SSC_CHAMELEON_DECODE_PROCESS_DATA_FAST:
            ssc_hash_decode_process_data_fast(in, out, state);
            state->process = SSC_CHAMELEON_DECODE_PROCESS_SIGNATURES_AND_DATA_FAST;
            break;

        case SSC_CHAMELEON_DECODE_PROCESS_FINISH:
            if (state->uncompressedChunkBytes) {
                if (ssc_hash_decode_attempt_copy(out, state->partialUncompressedChunk.as_bytes, (uint32_t) state->uncompressedChunkBytes))
                    return SSC_KERNEL_DECODE_STATE_STALL_ON_OUTPUT_BUFFER;
                state->uncompressedChunkBytes = 0;
            }
            remaining = in->size - in->position;
            if (remaining > state->endDataOverhead) {
                if (ssc_hash_decode_attempt_copy(out, in->pointer + in->position, (uint32_t) (remaining - state->endDataOverhead)))
                    return SSC_KERNEL_DECODE_STATE_STALL_ON_OUTPUT_BUFFER;
                in->position += (remaining - state->endDataOverhead);
            }
            state->process = SSC_CHAMELEON_DECODE_PROCESS_SIGNATURES_AND_DATA_FAST;
            return SSC_KERNEL_DECODE_STATE_FINISHED;

        default:
            return SSC_KERNEL_DECODE_STATE_ERROR;
    }

    return SSC_KERNEL_DECODE_STATE_READY;
}

SSC_FORCE_INLINE SSC_KERNEL_DECODE_STATE CHAMELEON_NAME(ssc_chameleon_decode_finish)(ssc_chameleon_decode_state *state) {
    return SSC_KERNEL_DECODE_STATE_READY;
}

#endif