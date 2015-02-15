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
 * 06/12/13 20:28
 *
 * ------------------
 * Argonaut algorithm
 * ------------------
 *
 * Author(s)
 * Guillaume Voirin (https://github.com/gpnuma)
 *
 * Description
 * Multiform compression algorithm
 */

#include "kernel_argonaut_encode.h"
#include "kernel_argonaut_dictionary.h"
#include "kernel_argonaut.h"

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE exitProcess(density_argonaut_encode_state *state, DENSITY_ARGONAUT_ENCODE_PROCESS process, DENSITY_KERNEL_ENCODE_STATE kernelEncodeState) {
    state->process = process;
    return kernelEncodeState;
}

DENSITY_FORCE_INLINE void density_argonaut_encode_prepare_new_signature(density_memory_location *restrict out, density_argonaut_encode_state *restrict state) {
    state->signaturesCount++;
    state->shift = 0;
    state->signature = (density_argonaut_signature *) (out->pointer);
    *state->signature = 0;

    out->pointer += sizeof(density_argonaut_signature);
    //out->available_bytes -= sizeof(density_argonaut_signature);
}

DENSITY_FORCE_INLINE void density_argonaut_encode_push_to_signature(density_memory_location *restrict out, density_argonaut_encode_state *restrict state, uint64_t content, uint_fast8_t bits) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    *(state->signature) |= (content << state->shift);
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    *(state->signature) |= (content << ((56 - (state->shift & ~0x7)) + (state->shift & 0x7)));
#endif

    state->shift += bits;

    if (state->shift & 0xffffffffffffffc0llu) {
        const uint8_t remainder = (uint_fast8_t) (state->shift & 0x3f);
        density_argonaut_encode_prepare_new_signature(out, state);
        if (remainder)
            density_argonaut_encode_push_to_signature(out, state, content >> (bits - remainder), remainder); //todo check big endian
    }
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE density_argonaut_encode_prepare_new_block(density_memory_location *restrict out, density_argonaut_encode_state *restrict state) {
    if (DENSITY_ARGONAUT_ENCODE_MINIMUM_OUTPUT_LOOKAHEAD > out->available_bytes)
        return DENSITY_KERNEL_ENCODE_STATE_STALL_ON_OUTPUT;

    switch (state->signaturesCount) {
        case DENSITY_ARGONAUT_PREFERRED_EFFICIENCY_CHECK_SIGNATURES:
            if (state->efficiencyChecked ^ 0x1) {
                state->efficiencyChecked = 1;
                return DENSITY_KERNEL_ENCODE_STATE_INFO_EFFICIENCY_CHECK;
            }
            break;
        case DENSITY_ARGONAUT_PREFERRED_BLOCK_SIGNATURES:
            state->signaturesCount = 0;
            state->efficiencyChecked = 0;

#if DENSITY_ENABLE_PARALLELIZABLE_DECOMPRESSIBLE_OUTPUT == DENSITY_YES
            if (state->resetCycle)
                state->resetCycle--;
            else {
                density_argonaut_dictionary_reset(&state->dictionary);
                state->resetCycle = DENSITY_DICTIONARY_PREFERRED_RESET_CYCLE - 1;
            }
#endif

            return DENSITY_KERNEL_ENCODE_STATE_INFO_NEW_BLOCK;
        default:
            break;
    }
    density_argonaut_encode_prepare_new_signature(out, state);

    return DENSITY_KERNEL_ENCODE_STATE_READY;
}

DENSITY_FORCE_INLINE density_argonaut_huffman_code density_argonaut_encode_fetch_form_rank_for_use(density_argonaut_encode_state *state, DENSITY_ARGONAUT_FORM form) {
    uint8_t rank = state->formStatistics[form].rank;

    state->formStatistics[form].usage++;
    if (rank) if (state->formStatistics[form].usage > state->formRanks[rank - 1].statistics->usage) {
        density_argonaut_form_statistics *replaced = state->formRanks[rank - 1].statistics;
        replaced->rank++;
        state->formRanks[rank - 1].statistics = &(state->formStatistics[form]);
        state->formRanks[rank].statistics = replaced;
        state->formStatistics[form].rank--;
    }

    return density_argonaut_form_codes[rank];
}

DENSITY_FORCE_INLINE void density_argonaut_encode_update_letter_rank(density_argonaut_dictionary_letter_entry *restrict letterEntry, density_argonaut_encode_state *restrict state) {
    letterEntry->usage++;
    if (letterEntry->rank) {
        uint8_t lowerRank = letterEntry->rank;
        uint8_t upperRank = lowerRank - (uint8_t) 1;
        if (letterEntry->usage > state->dictionary.letterRanks[upperRank]->usage) {
            density_argonaut_dictionary_letter_entry *swappedLetterEntry = state->dictionary.letterRanks[upperRank];
            letterEntry->rank = upperRank;
            state->dictionary.letterRanks[upperRank] = letterEntry;
            swappedLetterEntry->rank = lowerRank;
            state->dictionary.letterRanks[lowerRank] = swappedLetterEntry;
        }
    }
}

/*DENSITY_FORCE_INLINE void density_argonaut_encode_update_hash_rank(density_argonaut_dictionary_letter_entry *restrict letterEntry, density_argonaut_encode_state *restrict state) {
    letterEntry->usage++;
    if (letterEntry->rank) {
        uint8_t lowerRank = letterEntry->rank;
        uint8_t upperRank = lowerRank - (uint8_t) 1;
        if (letterEntry->usage > state->dictionary.hashRanks[upperRank]->usage) {
            density_argonaut_dictionary_letter_entry *swappedLetterEntry = state->dictionary.hashRanks[upperRank];
            letterEntry->rank = upperRank;
            state->dictionary.hashRanks[upperRank] = letterEntry;
            swappedLetterEntry->rank = lowerRank;
            state->dictionary.hashRanks[lowerRank] = swappedLetterEntry;
        }
    }
}*/

DENSITY_FORCE_INLINE void density_argonaut_encode_kernel(density_memory_location *restrict out, uint32_t *restrict hash, const uint32_t chunk, density_argonaut_encode_state *restrict state) {
    DENSITY_ARGONAUT_HASH_ALGORITHM(*hash, DENSITY_LITTLE_ENDIAN_32(chunk));
    //density_argonaut_dictionary_chunk_prediction_entry* predictions = &(state->dictionary.predictions[state->lastHash]);

    uint32_t *predictedChunk = &(state->dictionary.predictions[state->lastHash].next_chunk_prediction);
    /*bool afterBestRankedLetter = (((state->lastChunk & 0xFF00) >> 8) == (*state->dictionary.letterRanks)->letter);

    if (afterBestRankedLetter) {
        predictedChunk = &(predictions->next_special_prediction);
    } else {
        predictedChunk = &(predictions->next_chunk_prediction);
    }*/
    uint32_t chunk1;

    if (*predictedChunk ^ chunk) {
        density_argonaut_dictionary_chunk_entry *found = &state->dictionary.chunks[*hash];
        uint32_t *found_a = &found->chunk_a;
        if (*found_a ^ chunk) {
            uint32_t *found_b = &found->chunk_b;
            if (*found_b ^ chunk) {

                density_argonaut_huffman_code code = density_argonaut_encode_fetch_form_rank_for_use(state, DENSITY_ARGONAUT_FORM_ENCODED);
                density_argonaut_encode_push_to_signature(out, state, code.code, code.bitSize);

                /*if(density_argonaut_contains_value32(chunk, (*state->dictionary.letterRanks)->letter)) {
                }*/

                /*uint16_t *predictedBigram = &(state->dictionary.bigramPredictions[chunk & 0xFFFF].next_bigram_prediction);
                if(*predictedBigram == ((chunk & 0xFFFF0000) >> 16)) {
                    *(uint16_t*) (out->pointer) = (uint16_t)(chunk & 0xFFFF);
                    out->pointer += sizeof(uint16_t);
                    goto finish;
                }
                *predictedBigram = (uint16_t)((chunk & 0xFFFF0000) >> 16);*/


                //density_argonaut_encode_push_to_signature(out, state, DENSITY_ARGONAUT_SIGNATURE_FLAG_CHUNK, 2);
                /*for(uint_fast8_t letter = 0; letter < sizeof(uint32_t); letter ++) {
                    density_argonaut_huffman_codes[wordFound->rank].bitSize
                    density_argonaut_encode_push_to_signature(out, state, 0, rank + (uint8_t) 1);
                }*/
                //*(uint32_t *) (out->pointer) = chunk;
                //out->pointer += sizeof(uint32_t);
                //uint8_t lastLetter = (uint8_t)((state->lastChunk & 0xFF000000) >> 24);
                uint8_t letterA = (uint8_t) (chunk & 0xFF);
                uint8_t letterB = (uint8_t) ((chunk & 0xFF00) >> 8);
                uint8_t letterC = (uint8_t) ((chunk & 0xFF0000) >> 16);
                uint8_t letterD = (uint8_t) ((chunk & 0xFF000000) >> 24);

                /*uint8_t *predictedA = &state->dictionary.letterPredictions[lastLetter].next_letter_prediction;
                if(*predictedA == letterA) {
                    density_argonaut_encode_push_to_signature(out, state, 0x0, (uint8_t)1);
                } else {
                    density_argonaut_huffman_code codeA = density_argonaut_huffman_codes[state->dictionary.letters[letterA].rank];
                    density_argonaut_encode_push_to_signature(out, state, codeA.code, (uint8_t)1 + codeA.bitSize);
                }

                uint8_t *predictedB = &state->dictionary.letterPredictions[letterA].next_letter_prediction;
                if(*predictedB == letterB) {
                    density_argonaut_encode_push_to_signature(out, state, 0x0, (uint8_t)1);
                } else {
                    density_argonaut_huffman_code codeB = density_argonaut_huffman_codes[state->dictionary.letters[letterB].rank];
                    density_argonaut_encode_push_to_signature(out, state, codeB.code, (uint8_t)1 + codeB.bitSize);
                }

                uint8_t *predictedC = &state->dictionary.letterPredictions[letterB].next_letter_prediction;
                if(*predictedC == letterC) {
                    density_argonaut_encode_push_to_signature(out, state, 0x0, (uint8_t)1);
                } else {
                    density_argonaut_huffman_code codeC = density_argonaut_huffman_codes[state->dictionary.letters[letterC].rank];
                    density_argonaut_encode_push_to_signature(out, state, codeC.code, (uint8_t)1 + codeC.bitSize);
                }

                uint8_t *predictedD = &state->dictionary.letterPredictions[letterC].next_letter_prediction;
                if(*predictedD == letterD) {
                    density_argonaut_encode_push_to_signature(out, state, 0x0, (uint8_t)1);
                } else {
                    density_argonaut_huffman_code codeD = density_argonaut_huffman_codes[state->dictionary.letters[letterD].rank];
                    density_argonaut_encode_push_to_signature(out, state, codeD.code, (uint8_t)1 + codeD.bitSize);
                }

                *predictedA = letterA;
                *predictedB = letterB;
                *predictedC = letterC;
                *predictedD = letterD;*/

                density_argonaut_huffman_code codeA = density_argonaut_huffman_codes[state->dictionary.letters[letterA].rank];
                density_argonaut_huffman_code codeB = density_argonaut_huffman_codes[state->dictionary.letters[letterB].rank];
                density_argonaut_huffman_code codeC = density_argonaut_huffman_codes[state->dictionary.letters[letterC].rank];
                density_argonaut_huffman_code codeD = density_argonaut_huffman_codes[state->dictionary.letters[letterD].rank];

                density_argonaut_encode_push_to_signature(out, state, codeA.code, codeA.bitSize);
                density_argonaut_encode_push_to_signature(out, state, codeB.code, codeB.bitSize);
                density_argonaut_encode_push_to_signature(out, state, codeC.code, codeC.bitSize);
                density_argonaut_encode_push_to_signature(out, state, codeD.code, codeD.bitSize);

                density_argonaut_encode_update_letter_rank(&state->dictionary.letters[letterA], state);
                density_argonaut_encode_update_letter_rank(&state->dictionary.letters[letterB], state);
                density_argonaut_encode_update_letter_rank(&state->dictionary.letters[letterC], state);
                density_argonaut_encode_update_letter_rank(&state->dictionary.letters[letterD], state);
            } else {
                //found->usage_b++;
                //found->durability_b++;
                density_argonaut_huffman_code code = density_argonaut_encode_fetch_form_rank_for_use(state, DENSITY_ARGONAUT_FORM_DICTIONARY_B);
                density_argonaut_encode_push_to_signature(out, state, code.code, code.bitSize);
                //density_argonaut_encode_push_to_signature(out, state, 0x0, (uint8_t) (DENSITY_ARGONAUT_HASH_BITS - 8));
                *(uint16_t *) (out->pointer) = DENSITY_LITTLE_ENDIAN_16(*hash);
                out->pointer += sizeof(uint16_t);
                /**(out->pointer) = (uint8_t) (*hash & 0xFF);
                out->pointer += sizeof(uint8_t);*/

                /*uint8_t letter1 = (uint8_t)(*hash & 0xFF);
                uint8_t letter2 = (uint8_t)(*hash >> 8);

                if(letter2 == 0)
                    out->pointer += sizeof(uint8_t);
                else
                    out->pointer += sizeof(uint16_t);*/

                /*density_argonaut_huffman_code code1 = density_argonaut_huffman_codes[state->dictionary.hashes[letter1].rank];
                density_argonaut_huffman_code code2 = density_argonaut_huffman_codes[state->dictionary.hashes[letter2].rank];

                density_argonaut_encode_push_to_signature(out, state, code1.code, code1.bitSize);
                density_argonaut_encode_push_to_signature(out, state, code2.code, code2.bitSize);

                density_argonaut_encode_update_hash_rank(&state->dictionary.hashes[letter1], state);
                density_argonaut_encode_update_hash_rank(&state->dictionary.hashes[letter2], state);*/
            }
            //if(!--found->durability_b) {
            *found_b = *found_a;
            *found_a = chunk;
            //}
        } else {
            /*density_argonaut_dictionary_hash_entry* hashEntry = &state->dictionary.hashes[*hash];
            hashEntry->usage ++;
            bool exit = hashEntry->rank < 255;

            if(exit) {
                density_argonaut_encode_push_to_signature(out, state, 0x1, density_argonaut_encode_fetch_form_rank_for_use(state, DENSITY_ARGONAUT_FORM_HASH_RANKS));
                *out->pointer = hashEntry->rank;
                out->available_bytes--;
            }
            // Update usage and rank
            if (hashEntry->rank) {
                uint_fast8_t lowerRank = hashEntry->rank;
                uint_fast8_t upperRank = lowerRank - (uint_fast8_t) 1;
                if (state->dictionary.hashRanks[upperRank] == NULL) {
                    hashEntry->rank = upperRank;
                    state->dictionary.hashRanks[upperRank] = hashEntry;
                    state->dictionary.hashRanks[lowerRank] = NULL;
                } else if (hashEntry->usage > state->dictionary.hashRanks[upperRank]->usage) {
                    density_argonaut_dictionary_hash_entry *replaced = state->dictionary.hashRanks[upperRank];
                    hashEntry->rank = upperRank;
                    state->dictionary.hashRanks[upperRank] = hashEntry;
                    replaced->rank = lowerRank;
                    state->dictionary.hashRanks[lowerRank] = replaced;
                }
            }

            if(exit)
                goto finish;*/

            //found->usage_a++;
            //found->durability_a++;
            density_argonaut_huffman_code code = density_argonaut_encode_fetch_form_rank_for_use(state, DENSITY_ARGONAUT_FORM_DICTIONARY_A);
            density_argonaut_encode_push_to_signature(out, state, code.code, code.bitSize);
            //density_argonaut_encode_push_to_signature(out, state, 0x0, (uint8_t)(DENSITY_ARGONAUT_HASH_BITS - 8));
            *(uint16_t *) (out->pointer) = DENSITY_LITTLE_ENDIAN_16(*hash);
            out->pointer += sizeof(uint16_t);
            /**(out->pointer) = (uint8_t)(*hash & 0xFF);
            out->pointer += sizeof(uint8_t);*/

            /*uint8_t letter1 = (uint8_t)(*hash & 0xFF);
            uint8_t letter2 = (uint8_t)(*hash >> 8);

            density_argonaut_huffman_code code1 = density_argonaut_huffman_codes[state->dictionary.hashes[letter1].rank];
            density_argonaut_huffman_code code2 = density_argonaut_huffman_codes[state->dictionary.hashes[letter2].rank];

            density_argonaut_encode_push_to_signature(out, state, code1.code, code1.bitSize);
            density_argonaut_encode_push_to_signature(out, state, code2.code, code2.bitSize);

            density_argonaut_encode_update_hash_rank(&state->dictionary.hashes[letter1], state);
            density_argonaut_encode_update_hash_rank(&state->dictionary.hashes[letter2], state);*/
        }
        *predictedChunk = chunk;
    } else {
        density_argonaut_huffman_code code = density_argonaut_encode_fetch_form_rank_for_use(state, DENSITY_ARGONAUT_FORM_PREDICTIONS);
        density_argonaut_encode_push_to_signature(out, state, code.code, code.bitSize);
        //density_argonaut_encode_push_to_signature(out, state, DENSITY_ARGONAUT_SIGNATURE_FLAG_PREDICTED, 2);
    }
    finish:

    /*chunk1 = (state->lastChunk >> 8) | ((chunk & 0xFF) << 24);
    uint32_t chunk2 = (state->lastChunk >> 16) | ((chunk & 0xFFFF) << 16);
    uint32_t chunk3 = (state->lastChunk >> 24) | ((chunk & 0xFFFFFF) << 8);

    uint32_t h;
    (&state->dictionary.chunks[*hash])->chunk_a = chunk;
    DENSITY_ARGONAUT_HASH_ALGORITHM(h, DENSITY_LITTLE_ENDIAN_32(chunk1));
    (&state->dictionary.chunks[h])->chunk_a = chunk1;
    DENSITY_ARGONAUT_HASH_ALGORITHM(h, DENSITY_LITTLE_ENDIAN_32(chunk2));
    (&state->dictionary.chunks[h])->chunk_a = chunk2;
    DENSITY_ARGONAUT_HASH_ALGORITHM(h, DENSITY_LITTLE_ENDIAN_32(chunk3));
    (&state->dictionary.chunks[h])->chunk_a = chunk3;*/

        state->beforeLastHash = state->lastHash;
    state->lastHash = (uint16_t) *hash;
    state->lastChunk = chunk;
}

DENSITY_FORCE_INLINE void density_argonaut_encode_process_chunk(uint64_t *restrict chunk, density_memory_location *restrict in, density_memory_location *restrict out, uint32_t *restrict hash, density_argonaut_encode_state *restrict state) {
    *chunk = *(uint64_t *) (in->pointer);

    /*uint32_t chunk32 = (uint32_t)(*chunk & 0xFFFFFFFF);
    uint16_t hash64 = state->beforeLastHash ^ state->lastHash;

    uint32_t *predictedChunk = &(state->dictionary.predictions64[state->lastHash64].next_chunk_prediction);
    if (*predictedChunk ^ chunk32) {
        *predictedChunk = chunk32;
    } else {
        density_argonaut_huffman_code code = density_argonaut_encode_fetch_form_rank_for_use(state, DENSITY_ARGONAUT_FORM_PREDICTIONS);
        density_argonaut_encode_push_to_signature(out, state, code.code, code.bitSize);
        goto skip;
    }*/

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    density_argonaut_encode_kernel(out, hash, (uint32_t) (*chunk & 0xFFFFFFFF), state);
#endif
    density_argonaut_encode_kernel(out, hash, (uint32_t) (*chunk >> 32), state);
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    density_argonaut_encode_kernel(out, hash, (uint32_t) (*chunk & 0xFFFFFFFF), state);
#endif

    /*skip:
    state->lastHash64 = (uint16_t) hash64;*/
    in->pointer += sizeof(uint64_t);
}

DENSITY_FORCE_INLINE void density_argonaut_encode_process_span(uint64_t *restrict chunk, density_memory_location *restrict in, density_memory_location *restrict out, uint32_t *restrict hash, density_argonaut_encode_state *restrict state) {
    density_argonaut_encode_process_chunk(chunk, in, out, hash, state);
    density_argonaut_encode_process_chunk(chunk, in, out, hash, state);
    density_argonaut_encode_process_chunk(chunk, in, out, hash, state);
    density_argonaut_encode_process_chunk(chunk, in, out, hash, state);
}

DENSITY_FORCE_INLINE void density_argonaut_encode_process_unit(uint64_t *restrict chunk, density_memory_location *restrict in, density_memory_location *restrict out, uint32_t *restrict hash, density_argonaut_encode_state *restrict state) {
    density_argonaut_encode_process_span(chunk, in, out, hash, state);
    density_argonaut_encode_process_span(chunk, in, out, hash, state);
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE density_argonaut_encode_init(density_argonaut_encode_state *state) {
    state->signaturesCount = 0;
    state->efficiencyChecked = 0;
    density_argonaut_dictionary_reset(&state->dictionary);

#if DENSITY_ENABLE_PARALLELIZABLE_DECOMPRESSIBLE_OUTPUT == DENSITY_YES
    state->resetCycle = DENSITY_DICTIONARY_PREFERRED_RESET_CYCLE - 1;
#endif

    state->formStatistics[DENSITY_ARGONAUT_FORM_ENCODED].usage = 0;
    state->formStatistics[DENSITY_ARGONAUT_FORM_ENCODED].rank = 0;
    state->formStatistics[DENSITY_ARGONAUT_FORM_DICTIONARY_A].usage = 0;
    state->formStatistics[DENSITY_ARGONAUT_FORM_DICTIONARY_A].rank = 1;
    state->formStatistics[DENSITY_ARGONAUT_FORM_DICTIONARY_B].usage = 0;
    state->formStatistics[DENSITY_ARGONAUT_FORM_DICTIONARY_B].rank = 2;
    state->formStatistics[DENSITY_ARGONAUT_FORM_PREDICTIONS].usage = 0;
    state->formStatistics[DENSITY_ARGONAUT_FORM_PREDICTIONS].rank = 3;
    state->formStatistics[DENSITY_ARGONAUT_FORM_PREDICTIONS].usage = 0;
    //state->formStatistics[DENSITY_ARGONAUT_FORM_PREDICTIONS_64].rank = 4;
    //state->formStatistics[DENSITY_ARGONAUT_FORM_PREDICTIONS_64].usage = 0;

    state->formRanks[0].statistics = &state->formStatistics[DENSITY_ARGONAUT_FORM_ENCODED];
    state->formRanks[1].statistics = &state->formStatistics[DENSITY_ARGONAUT_FORM_DICTIONARY_A];
    state->formRanks[2].statistics = &state->formStatistics[DENSITY_ARGONAUT_FORM_DICTIONARY_B];
    state->formRanks[3].statistics = &state->formStatistics[DENSITY_ARGONAUT_FORM_PREDICTIONS];
    //state->formRanks[4].statistics = &state->formStatistics[DENSITY_ARGONAUT_FORM_PREDICTIONS_64];

    uint_fast8_t count = 0xff;
    do {
        state->dictionary.letters[count].letter = count;
        state->dictionary.letters[count].rank = count;
        state->dictionary.letterRanks[count] = &state->dictionary.letters[count];
    } while (count--);

    /*count = 0xff;
    do {
        state->dictionary.hashes[count].letter = count;
        state->dictionary.hashes[count].rank = count;
        state->dictionary.hashRanks[count] = &state->dictionary.hashes[count];
    } while (count--);*/

    /*count = 0xff;
    do {
        state->dictionary.hashRanks[count] = NULL;
    } while (count--);*/

    /*uint16_t diCount = 0xffff;
    do {
        state->dictionary.hashes[diCount].usage = 0;
        state->dictionary.hashes[diCount].rank = 255;
    } while (diCount--);*/

    state->lastHash = 0;
    state->lastChunk = 0;
    state->beforeLastHash = 0;

    return exitProcess(state, DENSITY_ARGONAUT_ENCODE_PROCESS_PREPARE_NEW_BLOCK, DENSITY_KERNEL_ENCODE_STATE_READY);
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE density_argonaut_encode_continue(density_memory_teleport *restrict in, density_memory_location *restrict out, density_argonaut_encode_state *restrict state, const density_bool flush) {
    DENSITY_KERNEL_ENCODE_STATE returnState;
    uint32_t hash;
    uint64_t chunk;
    density_byte *pointerOutBefore;
    density_memory_location *readMemoryLocation;

    // Dispatch
    switch (state->process) {
        case DENSITY_ARGONAUT_ENCODE_PROCESS_PREPARE_NEW_BLOCK:
            goto prepare_new_block;
        case DENSITY_ARGONAUT_ENCODE_PROCESS_READ_CHUNK:
            goto read_chunk;
        case DENSITY_ARGONAUT_ENCODE_PROCESS_CHECK_SIGNATURE_STATE:
            goto check_signature_state;
        default:
            return DENSITY_KERNEL_ENCODE_STATE_ERROR;
    }

    // Prepare new block
    prepare_new_block:
    if ((returnState = density_argonaut_encode_prepare_new_block(out, state)))
        return exitProcess(state, DENSITY_ARGONAUT_ENCODE_PROCESS_PREPARE_NEW_BLOCK, returnState);

    check_signature_state:
    if (DENSITY_ARGONAUT_ENCODE_MINIMUM_OUTPUT_LOOKAHEAD > out->available_bytes)
        return exitProcess(state, DENSITY_ARGONAUT_ENCODE_PROCESS_CHECK_SIGNATURE_STATE, DENSITY_KERNEL_ENCODE_STATE_STALL_ON_OUTPUT);

    // Try to read a complete chunk unit
    read_chunk:
    pointerOutBefore = out->pointer;
    if (!(readMemoryLocation = density_memory_teleport_read(in, DENSITY_ARGONAUT_ENCODE_PROCESS_UNIT_SIZE)))
        return exitProcess(state, DENSITY_ARGONAUT_ENCODE_PROCESS_READ_CHUNK, DENSITY_KERNEL_ENCODE_STATE_STALL_ON_INPUT);

    // Chunk was read properly, process
    density_argonaut_encode_process_unit(&chunk, readMemoryLocation, out, &hash, state);
    readMemoryLocation->available_bytes -= DENSITY_ARGONAUT_ENCODE_PROCESS_UNIT_SIZE;
    out->available_bytes -= (out->pointer - pointerOutBefore);

    // New loop
    goto check_signature_state;
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE density_argonaut_encode_finish(density_memory_teleport *restrict in, density_memory_location *restrict out, density_argonaut_encode_state *restrict state) {
    DENSITY_KERNEL_ENCODE_STATE returnState;
    uint32_t hash;
    uint64_t chunk;
    density_memory_location *readMemoryLocation;
    density_byte *pointerOutBefore;

    // Dispatch
    switch (state->process) {
        case DENSITY_ARGONAUT_ENCODE_PROCESS_PREPARE_NEW_BLOCK:
            goto prepare_new_block;
        case DENSITY_ARGONAUT_ENCODE_PROCESS_READ_CHUNK:
            goto read_chunk;
        case DENSITY_ARGONAUT_ENCODE_PROCESS_CHECK_SIGNATURE_STATE:
            goto check_signature_state;
        default:
            return DENSITY_KERNEL_ENCODE_STATE_ERROR;
    }

    // Prepare new block
    prepare_new_block:
    if ((returnState = density_argonaut_encode_prepare_new_block(out, state)))
        return exitProcess(state, DENSITY_ARGONAUT_ENCODE_PROCESS_PREPARE_NEW_BLOCK, returnState);

    check_signature_state:
    if (DENSITY_ARGONAUT_ENCODE_MINIMUM_OUTPUT_LOOKAHEAD > out->available_bytes)
        return exitProcess(state, DENSITY_ARGONAUT_ENCODE_PROCESS_CHECK_SIGNATURE_STATE, DENSITY_KERNEL_ENCODE_STATE_STALL_ON_OUTPUT);

    // Try to read a complete chunk unit
    read_chunk:
    pointerOutBefore = out->pointer;
    if (!(readMemoryLocation = density_memory_teleport_read(in, DENSITY_ARGONAUT_ENCODE_PROCESS_UNIT_SIZE)))
        goto step_by_step;

    // Chunk was read properly, process
    density_argonaut_encode_process_unit(&chunk, readMemoryLocation, out, &hash, state);
    readMemoryLocation->available_bytes -= DENSITY_ARGONAUT_ENCODE_PROCESS_UNIT_SIZE;
    goto exit;

    // Read step by step
    step_by_step:
    while (state->shift != bitsizeof(density_argonaut_signature) && (readMemoryLocation = density_memory_teleport_read(in, sizeof(uint32_t)))) {
        density_argonaut_encode_kernel(out, &hash, *(uint32_t *) (readMemoryLocation->pointer), state);
        readMemoryLocation->pointer += sizeof(uint32_t);
        readMemoryLocation->available_bytes -= sizeof(uint32_t);
    }
    exit:
    out->available_bytes -= (out->pointer - pointerOutBefore);

    // New loop
    if (density_memory_teleport_available(in) >= sizeof(uint32_t))
        goto check_signature_state;

    // Marker for decode loop exit
    density_argonaut_encode_push_to_signature(out, state, DENSITY_ARGONAUT_SIGNATURE_FLAG_CHUNK, 2);

    // Copy the remaining bytes
    density_memory_teleport_copy_remaining(in, out);

    return DENSITY_KERNEL_ENCODE_STATE_READY;
}