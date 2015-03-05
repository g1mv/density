/*
 * Centaurean Density
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
 * --------------
 * Lion algorithm
 * --------------
 *
 * Author(s)
 * Guillaume Voirin (https://github.com/gpnuma)
 *
 * Description
 * Multiform compression algorithm
 */

#include "kernel_lion_encode.h"

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE exitProcess(density_lion_encode_state *state, DENSITY_LION_ENCODE_PROCESS process, DENSITY_KERNEL_ENCODE_STATE kernelEncodeState) {
    state->process = process;
    return kernelEncodeState;
}

DENSITY_FORCE_INLINE void density_lion_encode_prepare_new_signature(density_memory_location *restrict out, density_lion_encode_state *restrict state) {
    state->signaturesCount++;
    state->shift = 0;
    state->signature = (density_lion_signature *) (out->pointer);
    *state->signature = 0;

    out->pointer += sizeof(density_lion_signature);
}

DENSITY_FORCE_INLINE void density_lion_encode_push_to_signature(density_memory_location *restrict out, density_lion_encode_state *restrict state, uint64_t content, uint_fast8_t bits) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    *(state->signature) |= (content << state->shift);
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    *(state->signature) |= (content << ((56 - (state->shift & ~0x7)) + (state->shift & 0x7)));
#endif

    state->shift += bits;

    if (state->shift & 0xFFFFFFFFFFFFFFC0llu) {
        const uint8_t remainder = (uint_fast8_t) (state->shift & 0x3F);
        density_lion_encode_prepare_new_signature(out, state);
        if (remainder)
            density_lion_encode_push_to_signature(out, state, content >> (bits - remainder), remainder); //todo check big endian
    }
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE density_lion_encode_prepare_new_block(density_memory_location *restrict out, density_lion_encode_state *restrict state) {
    if (DENSITY_LION_ENCODE_MINIMUM_OUTPUT_LOOKAHEAD > out->available_bytes)
        return DENSITY_KERNEL_ENCODE_STATE_STALL_ON_OUTPUT;

    switch (state->signaturesCount) {
        case DENSITY_LION_PREFERRED_EFFICIENCY_CHECK_SIGNATURES:
            if (state->efficiencyChecked ^ 0x1) {
                state->efficiencyChecked = 1;
                return DENSITY_KERNEL_ENCODE_STATE_INFO_EFFICIENCY_CHECK;
            }
            break;
        case DENSITY_LION_PREFERRED_BLOCK_SIGNATURES:
            state->signaturesCount = 0;
            state->efficiencyChecked = 0;

#if DENSITY_ENABLE_PARALLELIZABLE_DECOMPRESSIBLE_OUTPUT == DENSITY_YES
            if (state->resetCycle)
                state->resetCycle--;
            else {
                density_lion_dictionary_reset(&state->dictionary);
                state->resetCycle = DENSITY_DICTIONARY_PREFERRED_RESET_CYCLE - 1;
            }
#endif

            return DENSITY_KERNEL_ENCODE_STATE_INFO_NEW_BLOCK;
        default:
            break;
    }
    density_lion_encode_prepare_new_signature(out, state);

    return DENSITY_KERNEL_ENCODE_STATE_READY;
}

DENSITY_FORCE_INLINE density_lion_entropy_code density_lion_encode_fetch_form_rank_for_use(density_lion_encode_state *state, DENSITY_LION_FORM form) {
    density_lion_form_statistics *stats = &state->formStatistics[form];

    const uint8_t rank = stats->rank;
    if (density_unlikely(rank)) {
        density_lion_form_rank *rankCurrent = &state->formRanks[rank];
        density_lion_form_rank *rankUpper = &state->formRanks[rank - 1];
        if (density_unlikely(stats->usage > rankUpper->statistics->usage)) {
            density_lion_form_statistics *replaced = rankUpper->statistics;
            replaced->rank++;
            stats->rank--;
            rankUpper->statistics = stats;
            rankCurrent->statistics = replaced;
        }
        stats->usage++;
        return density_lion_form_entropy_codes[rank];
    } else {
        stats->usage++;
        return density_lion_form_entropy_codes[0];
    }
}

DENSITY_FORCE_INLINE void density_lion_encode_kernel(density_memory_location *restrict out, uint32_t *restrict hash, const uint32_t chunk, density_lion_encode_state *restrict state) {
    DENSITY_LION_HASH_ALGORITHM(*hash, DENSITY_LITTLE_ENDIAN_32(chunk));
    uint32_t *predictedChunk = &(state->dictionary.predictions[state->lastHash].next_chunk_prediction);

    if (*predictedChunk ^ chunk) {
        density_lion_dictionary_chunk_entry *found = &state->dictionary.chunks[*hash];
        uint32_t *found_a = &found->chunk_a;
        if (*found_a ^ chunk) {
            uint32_t *found_b = &found->chunk_b;
            if (*found_b ^ chunk) {
                density_lion_entropy_code code = density_lion_encode_fetch_form_rank_for_use(state, DENSITY_LION_FORM_SECONDARY);
                density_lion_encode_push_to_signature(out, state, code.value, code.bitLength);

                const uint32_t chunkRS8 = chunk >> 8;
                const uint32_t chunkRS16 = chunk >> 16;
                const uint32_t chunkRS24 = chunk >> 24;

                const uint16_t bigramP = (uint16_t) ((state->lastChunk >> 24) | ((chunk & 0xFF) << 8));
                const uint16_t bigramA = (uint16_t) (chunk & 0xFFFF);
                const uint16_t bigramB = (uint16_t) (chunkRS8 & 0xFFFF);
                const uint16_t bigramC = (uint16_t) (chunkRS16 & 0xFFFF);

                const uint8_t hashP = (uint8_t) (((bigramP * DENSITY_LION_HASH32_MULTIPLIER) >> (32 - DENSITY_LION_DIGRAM_HASH_BITS)));
                const uint8_t hashA = (uint8_t) (((bigramA * DENSITY_LION_HASH32_MULTIPLIER) >> (32 - DENSITY_LION_DIGRAM_HASH_BITS)));
                const uint8_t hashB = (uint8_t) (((bigramB * DENSITY_LION_HASH32_MULTIPLIER) >> (32 - DENSITY_LION_DIGRAM_HASH_BITS)));
                const uint8_t hashC = (uint8_t) (((bigramC * DENSITY_LION_HASH32_MULTIPLIER) >> (32 - DENSITY_LION_DIGRAM_HASH_BITS)));

                density_lion_dictionary_bigram_entry* bigramEntryA = &state->dictionary.bigrams[hashA];
                density_lion_dictionary_bigram_entry* bigramEntryC = &state->dictionary.bigrams[hashC];

                density_lion_encode_push_to_signature(out, state, 0x1, 2);
                if (bigramEntryA->bigram == bigramA) {
                    *(out->pointer) = hashA;
                    out->pointer++;
                } else {
                    const uint8_t letterA = (uint8_t) (chunk & 0xFF);
                    const uint8_t letterB = (uint8_t) (chunkRS8 & 0xFF);
                }

                if (bigramEntryC->bigram == bigramC) {
                    *(out->pointer) = hashC;
                    out->pointer++;
                } else {
                    const uint8_t letterC = (uint8_t) (chunkRS16 & 0xFF);
                    const uint8_t letterD = (uint8_t) (chunkRS24 & 0xFF);
                }

                state->dictionary.bigrams[hashP].bigram = bigramP;
                bigramEntryA->bigram = bigramA;
                state->dictionary.bigrams[hashB].bigram = bigramB;
                bigramEntryC->bigram = bigramC;
            } else {
                density_lion_entropy_code code = density_lion_encode_fetch_form_rank_for_use(state, DENSITY_LION_FORM_DICTIONARY_B);
                density_lion_encode_push_to_signature(out, state, code.value, code.bitLength);

                *(uint16_t *) (out->pointer) = DENSITY_LITTLE_ENDIAN_16(*hash);
                out->pointer += sizeof(uint16_t);
            }
            *found_b = *found_a;
            *found_a = chunk;
        } else {
            density_lion_entropy_code code = density_lion_encode_fetch_form_rank_for_use(state, DENSITY_LION_FORM_DICTIONARY_A);
            density_lion_encode_push_to_signature(out, state, code.value, code.bitLength);

            *(uint16_t *) (out->pointer) = DENSITY_LITTLE_ENDIAN_16(*hash);
            out->pointer += sizeof(uint16_t);
        }
        *predictedChunk = chunk;
    } else {
        density_lion_entropy_code code = density_lion_encode_fetch_form_rank_for_use(state, DENSITY_LION_FORM_PREDICTIONS);
        density_lion_encode_push_to_signature(out, state, code.value, code.bitLength);
    }

    state->lastHash = *hash;
    state->lastChunk = chunk;
}

DENSITY_FORCE_INLINE void density_lion_encode_process_chunk(uint64_t *restrict chunk, density_memory_location *restrict in, density_memory_location *restrict out, uint32_t *restrict hash, density_lion_encode_state *restrict state) {
    *chunk = *(uint64_t *) (in->pointer);

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    density_lion_encode_kernel(out, hash, (uint32_t) (*chunk & 0xFFFFFFFF), state);
#endif
    density_lion_encode_kernel(out, hash, (uint32_t) (*chunk >> 32), state);
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    density_lion_encode_kernel(out, hash, (uint32_t) (*chunk & 0xFFFFFFFF), state);
#endif

    in->pointer += sizeof(uint64_t);
}

DENSITY_FORCE_INLINE void density_lion_encode_process_span(uint64_t *restrict chunk, density_memory_location *restrict in, density_memory_location *restrict out, uint32_t *restrict hash, density_lion_encode_state *restrict state) {
    density_lion_encode_process_chunk(chunk, in, out, hash, state);
    density_lion_encode_process_chunk(chunk, in, out, hash, state);
    density_lion_encode_process_chunk(chunk, in, out, hash, state);
    density_lion_encode_process_chunk(chunk, in, out, hash, state);
}

DENSITY_FORCE_INLINE void density_lion_encode_process_unit(uint64_t *restrict chunk, density_memory_location *restrict in, density_memory_location *restrict out, uint32_t *restrict hash, density_lion_encode_state *restrict state) {
    density_lion_encode_process_span(chunk, in, out, hash, state);
    density_lion_encode_process_span(chunk, in, out, hash, state);
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE density_lion_encode_init(density_lion_encode_state *state) {
    state->signaturesCount = 0;
    state->efficiencyChecked = 0;
    density_lion_dictionary_reset(&state->dictionary);

#if DENSITY_ENABLE_PARALLELIZABLE_DECOMPRESSIBLE_OUTPUT == DENSITY_YES
    state->resetCycle = DENSITY_DICTIONARY_PREFERRED_RESET_CYCLE - 1;
#endif

    state->formStatistics[DENSITY_LION_FORM_PREDICTIONS].usage = 0;
    state->formStatistics[DENSITY_LION_FORM_PREDICTIONS].rank = 3;
    state->formStatistics[DENSITY_LION_FORM_DICTIONARY_A].usage = 0;
    state->formStatistics[DENSITY_LION_FORM_DICTIONARY_A].rank = 1;
    state->formStatistics[DENSITY_LION_FORM_DICTIONARY_B].usage = 0;
    state->formStatistics[DENSITY_LION_FORM_DICTIONARY_B].rank = 2;
    state->formStatistics[DENSITY_LION_FORM_SECONDARY].usage = 0;
    state->formStatistics[DENSITY_LION_FORM_SECONDARY].rank = 0;

    state->formRanks[0].statistics = &state->formStatistics[DENSITY_LION_FORM_SECONDARY];
    state->formRanks[1].statistics = &state->formStatistics[DENSITY_LION_FORM_DICTIONARY_A];
    state->formRanks[2].statistics = &state->formStatistics[DENSITY_LION_FORM_DICTIONARY_B];
    state->formRanks[3].statistics = &state->formStatistics[DENSITY_LION_FORM_PREDICTIONS];

    /*uint_fast8_t count = 0xFF;
    do {
        state->dictionary.letters[count].letter = count;
        state->dictionary.letters[count].rank = count;
        state->dictionary.letterRanks[count] = &state->dictionary.letters[count];
        //state->dictionary.altchunks[count].hash = count;
    } while (count--);*/

    state->lastHash = 0;
    state->lastChunk = 0;

    return exitProcess(state, DENSITY_LION_ENCODE_PROCESS_PREPARE_NEW_BLOCK, DENSITY_KERNEL_ENCODE_STATE_READY);
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE density_lion_encode_continue(density_memory_teleport *restrict in, density_memory_location *restrict out, density_lion_encode_state *restrict state, const density_bool flush) {
    DENSITY_KERNEL_ENCODE_STATE returnState;
    uint32_t hash;
    uint64_t chunk;
    density_byte *pointerOutBefore;
    density_memory_location *readMemoryLocation;

    // Dispatch
    switch (state->process) {
        case DENSITY_LION_ENCODE_PROCESS_PREPARE_NEW_BLOCK:
            goto prepare_new_block;
        case DENSITY_LION_ENCODE_PROCESS_READ_CHUNK:
            goto read_chunk;
        case DENSITY_LION_ENCODE_PROCESS_CHECK_SIGNATURE_STATE:
            goto check_signature_state;
        default:
            return DENSITY_KERNEL_ENCODE_STATE_ERROR;
    }

    // Prepare new block
    prepare_new_block:
    if ((returnState = density_lion_encode_prepare_new_block(out, state)))
        return exitProcess(state, DENSITY_LION_ENCODE_PROCESS_PREPARE_NEW_BLOCK, returnState);

    check_signature_state:
    if (DENSITY_LION_ENCODE_MINIMUM_OUTPUT_LOOKAHEAD > out->available_bytes)
        return exitProcess(state, DENSITY_LION_ENCODE_PROCESS_CHECK_SIGNATURE_STATE, DENSITY_KERNEL_ENCODE_STATE_STALL_ON_OUTPUT);

    // Try to read a complete chunk unit
    read_chunk:
    pointerOutBefore = out->pointer;
    if (!(readMemoryLocation = density_memory_teleport_read(in, DENSITY_LION_ENCODE_PROCESS_UNIT_SIZE)))
        return exitProcess(state, DENSITY_LION_ENCODE_PROCESS_READ_CHUNK, DENSITY_KERNEL_ENCODE_STATE_STALL_ON_INPUT);

    // Chunk was read properly, process
    density_lion_encode_process_unit(&chunk, readMemoryLocation, out, &hash, state);
    readMemoryLocation->available_bytes -= DENSITY_LION_ENCODE_PROCESS_UNIT_SIZE;
    out->available_bytes -= (out->pointer - pointerOutBefore);

    // New loop
    goto check_signature_state;
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE density_lion_encode_finish(density_memory_teleport *restrict in, density_memory_location *restrict out, density_lion_encode_state *restrict state) {
    DENSITY_KERNEL_ENCODE_STATE returnState;
    uint32_t hash;
    uint64_t chunk;
    density_memory_location *readMemoryLocation;
    density_byte *pointerOutBefore;

    // Dispatch
    switch (state->process) {
        case DENSITY_LION_ENCODE_PROCESS_PREPARE_NEW_BLOCK:
            goto prepare_new_block;
        case DENSITY_LION_ENCODE_PROCESS_READ_CHUNK:
            goto read_chunk;
        case DENSITY_LION_ENCODE_PROCESS_CHECK_SIGNATURE_STATE:
            goto check_signature_state;
        default:
            return DENSITY_KERNEL_ENCODE_STATE_ERROR;
    }

    // Prepare new block
    prepare_new_block:
    if ((returnState = density_lion_encode_prepare_new_block(out, state)))
        return exitProcess(state, DENSITY_LION_ENCODE_PROCESS_PREPARE_NEW_BLOCK, returnState);

    check_signature_state:
    if (DENSITY_LION_ENCODE_MINIMUM_OUTPUT_LOOKAHEAD > out->available_bytes)
        return exitProcess(state, DENSITY_LION_ENCODE_PROCESS_CHECK_SIGNATURE_STATE, DENSITY_KERNEL_ENCODE_STATE_STALL_ON_OUTPUT);

    // Try to read a complete chunk unit
    read_chunk:
    pointerOutBefore = out->pointer;
    if (!(readMemoryLocation = density_memory_teleport_read(in, DENSITY_LION_ENCODE_PROCESS_UNIT_SIZE)))
        goto step_by_step;

    // Chunk was read properly, process
    density_lion_encode_process_unit(&chunk, readMemoryLocation, out, &hash, state);
    readMemoryLocation->available_bytes -= DENSITY_LION_ENCODE_PROCESS_UNIT_SIZE;
    goto exit;

    // Read step by step
    step_by_step:
    while (state->shift != density_bitsizeof(density_lion_signature) && (readMemoryLocation = density_memory_teleport_read(in, sizeof(uint32_t)))) {
        density_lion_encode_kernel(out, &hash, *(uint32_t *) (readMemoryLocation->pointer), state);
        readMemoryLocation->pointer += sizeof(uint32_t);
        readMemoryLocation->available_bytes -= sizeof(uint32_t);
    }
    exit:
    out->available_bytes -= (out->pointer - pointerOutBefore);

    // New loop
    if (density_memory_teleport_available(in) >= sizeof(uint32_t))
        goto check_signature_state;

    // Marker for decode loop exit
    density_lion_encode_push_to_signature(out, state, DENSITY_LION_SIGNATURE_FLAG_CHUNK, 2);

    // Copy the remaining bytes
    density_memory_teleport_copy_remaining(in, out);

    //done_encoding(out, state);

    return DENSITY_KERNEL_ENCODE_STATE_READY;
}