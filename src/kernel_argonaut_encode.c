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
 * 9/02/15 19:22
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
#include "memory_location.h"

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE exitProcess(density_argonaut_encode_state *state, DENSITY_ARGONAUT_ENCODE_PROCESS process, DENSITY_KERNEL_ENCODE_STATE kernelEncodeState) {
    state->process = process;
    return kernelEncodeState;
}

DENSITY_FORCE_INLINE void density_argonaut_encode_write_to_signature(density_mandala_encode_state *state, uint8_t content) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    *(state->signature) |= ((uint64_t) content) << state->shift;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    *(state->signature) |= ((uint64_t) content) << ((56 - (state->shift & ~0x7)) + (state->shift & 0x7));
#endif
}

DENSITY_FORCE_INLINE void density_argonaut_encode_prepare_new_signature(density_memory_location *restrict out, density_argonaut_encode_state *restrict state) {
    state->signaturesCount++;
    state->shift = 0;
    state->signature = (density_argonaut_signature *) (out->pointer);
    *state->signature = 0;

    out->pointer += sizeof(density_argonaut_signature);
    out->available_bytes -= sizeof(density_argonaut_signature);
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

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE density_argonaut_encode_init(density_argonaut_encode_state *state) {
    state->signaturesCount = 0;
    state->efficiencyChecked = 0;
    density_argonaut_dictionary_reset(&state->dictionary);

#if DENSITY_ENABLE_PARALLELIZABLE_DECOMPRESSIBLE_OUTPUT == DENSITY_YES
    state->resetCycle = DENSITY_DICTIONARY_PREFERRED_RESET_CYCLE - 1;
#endif

    state->formStatistics[DENSITY_ARGONAUT_FORM_PREDICTIONS].usage = 0;
    state->formStatistics[DENSITY_ARGONAUT_FORM_PREDICTIONS].rank = 3;
    state->formStatistics[DENSITY_ARGONAUT_FORM_RANK].usage = 0;
    state->formStatistics[DENSITY_ARGONAUT_FORM_RANK].rank = 1;
    state->formStatistics[DENSITY_ARGONAUT_FORM_DICTIONARY].usage = 0;
    state->formStatistics[DENSITY_ARGONAUT_FORM_DICTIONARY].rank = 0;
    state->formStatistics[DENSITY_ARGONAUT_FORM_ENCODED].usage = 0;
    state->formStatistics[DENSITY_ARGONAUT_FORM_ENCODED].rank = 2;

    state->formRanks[0].statistics = &state->formStatistics[DENSITY_ARGONAUT_FORM_DICTIONARY];
    state->formRanks[1].statistics = &state->formStatistics[DENSITY_ARGONAUT_FORM_RANK];
    state->formRanks[2].statistics = &state->formStatistics[DENSITY_ARGONAUT_FORM_ENCODED];
    state->formRanks[3].statistics = &state->formStatistics[DENSITY_ARGONAUT_FORM_PREDICTIONS];

    uint8_t count = 0xFF;
    do {
        state->dictionary.letters[count].letter = count;
        state->dictionary.letters[count].rank = count;
        state->dictionary.letterRanks[count] = &state->dictionary.letters[count];
    } while (count--);

    uint16_t dCount = 0xFFFF;
    do {
        state->dictionary.wordsA[dCount].word.as_uint64_t = 0;
        state->dictionary.wordsA[dCount].usage = 0;
        state->dictionary.wordsA[dCount].rank = 255;
        state->dictionary.wordsB[dCount].word.as_uint64_t = 0;
        state->dictionary.wordsB[dCount].usage = 0;
        state->dictionary.wordsB[dCount].rank = 255;
    } while (dCount--);

    count = 0xFF;
    do {
        state->dictionary.wordsA[count].rank = count;
        state->dictionary.wordRanks[count] = &state->dictionary.wordsA[count];
    } while (count--);

    return exitProcess(state, DENSITY_ARGONAUT_ENCODE_PROCESS_PREPARE_NEW_BLOCK, DENSITY_KERNEL_ENCODE_STATE_READY);
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE density_argonaut_encode_continue(density_memory_teleport *restrict in, density_memory_location *restrict out, density_argonaut_encode_state *restrict state) {
    DENSITY_KERNEL_ENCODE_STATE returnState;
    density_byte *pointerOutBefore;
    density_memory_location *readMemoryLocation;

    // Dispatch
    switch (state->process) {
        case DENSITY_ARGONAUT_ENCODE_PROCESS_PREPARE_NEW_BLOCK:
            goto prepare_new_block;
        case DENSITY_ARGONAUT_ENCODE_PROCESS_CHECK_SIGNATURE_STATE:;//goto check_signature_state;
        case DENSITY_ARGONAUT_ENCODE_PROCESS_READ_CHUNK:
            goto read_chunk;
        default:
            return DENSITY_KERNEL_ENCODE_STATE_ERROR;
    }

    // Prepare new block
    prepare_new_block:
    if ((returnState = density_argonaut_encode_prepare_new_block(out, state)))
        return exitProcess(state, DENSITY_ARGONAUT_ENCODE_PROCESS_PREPARE_NEW_BLOCK, returnState);

    // Read chunk
    read_chunk:
    if (!(readMemoryLocation = density_memory_teleport_read(in, 256)))
        return exitProcess(state, DENSITY_ARGONAUT_ENCODE_PROCESS_READ_CHUNK, DENSITY_KERNEL_ENCODE_STATE_STALL_ON_INPUT);

    // Fetch words
    uint_fast16_t count = 0;
    uint8_t letter = readMemoryLocation->pointer[count];

    loop:
    //state->word.letters[state->word.length ++] = letter;
    bool startedWithSeparator = (letter == state->dictionary.letterRanks[0]->letter);

    while (true) {
        density_argonaut_dictionary_letter_entry *letterEntry = &state->dictionary.letters[letter];

        // Stop if the word started with the separator letter and a different letter has been read
        if (startedWithSeparator && letterEntry->rank)
            break;

        // Update letter usage and rank
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

        if (!readMemoryLocation->available_bytes)
            return exitProcess(state, DENSITY_ARGONAUT_ENCODE_PROCESS_READ_CHUNK, DENSITY_KERNEL_ENCODE_STATE_STALL_ON_INPUT);

        state->word.letters[state->word.length++] = letter;
        letter = readMemoryLocation->pointer[++count];

        // Stop if word length has reached the maximum of 8 letters
        if (state->word.length == 8)
            break;

        // Stop if we found the separator and the word didn't begin with a separator
        if (!startedWithSeparator && !letterEntry->rank)
            break;
    }

    // Masking
    uint64_t word = state->word.as_uint64_t;
    if (state->word.length < 8)
        word &= (0xFFFFFFFFFFFFFFFFllu >> (64 - state->word.length * 8));

    // Check predictions
    if (state->word.length <= 4) {
        if (state->dictionary.wordPredictions[state->lastByteHash].next_word.as_uint64_t == word) {
            uint8_t rank = state->formStatistics[DENSITY_ARGONAUT_FORM_PREDICTIONS].rank;
            ////outputBits += rank + 1;
            state->formStatistics[DENSITY_ARGONAUT_FORM_PREDICTIONS].usage++;
            if (rank) if (state->formStatistics[DENSITY_ARGONAUT_FORM_PREDICTIONS].usage > state->formRanks[rank - 1].statistics->usage) {
                density_argonaut_form_statistics *replaced = state->formRanks[rank - 1].statistics;
                replaced->rank++;
                state->formRanks[rank - 1].statistics = &state->formStatistics[DENSITY_ARGONAUT_FORM_PREDICTIONS];
                state->formRanks[rank].statistics = replaced;
                state->formStatistics[DENSITY_ARGONAUT_FORM_PREDICTIONS].rank--;
            }
            goto finish;
        } else {
            state->dictionary.wordPredictions[state->lastByteHash].next_word.as_uint64_t = word;
        }
    }

    // Word hashes
    uint16_t hash16 = (uint16_t) ((DENSITY_ARGONAUT_HASH64_MULTIPLIER * word) >> (64 - DENSITY_ARGONAUT_DICTIONARY_BITS));
    uint8_t hash8 = (uint8_t) (hash16 ^ (hash16 >> 8));

    // Double dictionary search
    bool alternateSource = false;
    density_argonaut_dictionary_word_entry *wordFound = &state->dictionary.wordsA[hash16];
    if (wordFound->word.as_uint64_t != word) {
        wordFound = &state->dictionary.wordsB[hash16];
        alternateSource = true;
    }
    if (wordFound->word.as_uint64_t == word) {
        wordFound->usage++;
        wordFound->durability++;

        // Update usage and rank
        if (wordFound->rank) {
            uint8_t lowerRank = wordFound->rank;
            uint8_t upperRank = lowerRank - (uint8_t) 1;
            if (wordFound->usage > state->dictionary.wordRanks[upperRank]->usage) {
                density_argonaut_dictionary_word_entry *replaced = state->dictionary.wordRanks[upperRank];
                wordFound->rank = upperRank;
                state->dictionary.wordRanks[upperRank] = wordFound;
                replaced->rank = lowerRank;
                state->dictionary.wordRanks[lowerRank] = replaced;
            }
        }

        // Check if we have a 8 bit rank
        if (wordFound->rank < 255) {
            ////uint8_t add = density_argonaut_huffman_codes[found16->rank].bitSize;
            uint8_t rank = state->formStatistics[DENSITY_ARGONAUT_FORM_RANK].rank;
            ////outputBits += rank + 1 + add;
            state->formStatistics[DENSITY_ARGONAUT_FORM_RANK].usage++;
            if (rank) if (state->formStatistics[DENSITY_ARGONAUT_FORM_RANK].usage > state->formRanks[rank - 1].statistics->usage) {
                density_argonaut_form_statistics *swap = state->formRanks[rank - 1].statistics;
                swap->rank++;
                state->formRanks[rank - 1].statistics = &state->formStatistics[DENSITY_ARGONAUT_FORM_RANK];
                state->formRanks[rank].statistics = swap;
                state->formStatistics[DENSITY_ARGONAUT_FORM_RANK].rank--;
            }
            goto finish;
        }

        uint8_t rank = state->formStatistics[DENSITY_ARGONAUT_FORM_DICTIONARY].rank;
        ////outputBits += rank + 1 + DENSITY_ARGONAUT_DICTIONARY_BITS;
        state->formStatistics[DENSITY_ARGONAUT_FORM_DICTIONARY].usage++;
        if (rank) if (state->formStatistics[DENSITY_ARGONAUT_FORM_DICTIONARY].usage > state->formRanks[rank - 1].statistics->usage) {
            density_argonaut_form_statistics *swap = state->formRanks[rank - 1].statistics;
            swap->rank++;
            state->formRanks[rank - 1].statistics = &state->formStatistics[DENSITY_ARGONAUT_FORM_DICTIONARY];
            state->formRanks[rank].statistics = swap;
            state->formStatistics[DENSITY_ARGONAUT_FORM_DICTIONARY].rank--;
        }
        goto finish;
    } else {
        if (wordFound->durability)
            wordFound->durability--;
        else {
            wordFound->word.as_uint64_t = word;
            wordFound->rank = 255;
            wordFound->usage = 1;
            wordFound->durability = 0;

            // Swap entries
            if (alternateSource) {
                density_argonaut_dictionary_word_entry replaced = state->dictionary.wordsA[hash16];
                state->dictionary.wordsA[hash16] = *wordFound;
                state->dictionary.wordsB[hash16] = replaced;
            }
        }

        // Check word inner predictions
        bool encode = true;
        uint32_t tot = 0;
        tot += density_argonaut_huffman_codes[state->dictionary.letters[state->word.letters[0]].rank].bitSize;
        for (int wordLetter = 1; wordLetter < state->word.length; wordLetter++) {
            if (wordLetter == 5 && state->dictionary.fourgramPredictions[(((uint32_t) (state->word.as_uint64_t & 0xFFFFFFFF)) * DENSITY_ARGONAUT_HASH32_MULTIPLIER) >> 16].next_part == (uint32_t) ((state->word.as_uint64_t & 0xFFFFFFFF00000000) >> 8)) {
                tot += 2;
                wordLetter += 3;
            } else if (state->dictionary.unigramPredictions[(uint8_t) state->word.letters[wordLetter - 1]].next_letter == state->word.letters[wordLetter]) {
                tot += 2;
            } else if (wordLetter >= 2 && state->dictionary.bigramPredictions[((uint16_t) (state->word.letters[wordLetter - 2]) << 8) + (uint16_t) state->word.letters[wordLetter - 1]].next_letter == state->word.letters[wordLetter]) {
                tot += 2;
            } else {
                uint8_t add = 2 + density_argonaut_huffman_codes[state->dictionary.letters[state->word.letters[wordLetter]].rank].bitSize;
                tot += add;
            }
        }

        // Update word inner predictions
        for (int wordLetter = 1; wordLetter < state->word.length; wordLetter++)
            state->dictionary.unigramPredictions[(uint8_t) state->word.letters[wordLetter - 1]].next_letter = (uint8_t) state->word.letters[wordLetter];

        if (state->word.length >= 2)
            for (int wordLetter = 2; wordLetter < state->word.length; wordLetter++)
                state->dictionary.bigramPredictions[((uint16_t) (state->word.letters[wordLetter - 2]) << 8) + (uint16_t) state->word.letters[wordLetter - 1]].next_letter = (uint8_t) state->word.letters[wordLetter];

        if (state->word.length == 5)
            state->dictionary.fourgramPredictions[((uint32_t) (state->word.as_uint64_t & 0xFFFFFFFF) * DENSITY_ARGONAUT_HASH32_MULTIPLIER) >> 16].next_part = (uint32_t) ((state->word.as_uint64_t & 0xFFFFFFFF00000000) >> 8);

        uint8_t rank = state->formStatistics[DENSITY_ARGONAUT_FORM_ENCODED].rank;
        ////uint32_t add = (rank + 1 + tot);
        ////outputBits += add;
        state->formStatistics[DENSITY_ARGONAUT_FORM_ENCODED].usage++;
        if (rank) if (state->formStatistics[DENSITY_ARGONAUT_FORM_ENCODED].usage > state->formRanks[rank - 1].statistics->usage) {
            density_argonaut_form_statistics *swap = state->formRanks[rank - 1].statistics;
            swap->rank++;
            state->formRanks[rank - 1].statistics = &state->formStatistics[DENSITY_ARGONAUT_FORM_ENCODED];
            state->formRanks[rank].statistics = swap;
            state->formStatistics[DENSITY_ARGONAUT_FORM_ENCODED].rank--;
        }

        goto  finish;
    }

    finish:

    state->lastByteHash = hash8;

    goto loop;
    return DENSITY_KERNEL_ENCODE_STATE_READY;
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE density_argonaut_encode_finish(density_memory_teleport *restrict in, density_memory_location *restrict out, density_argonaut_encode_state *restrict state) {
    return DENSITY_KERNEL_ENCODE_STATE_READY;
}