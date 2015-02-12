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
#include "kernel_argonaut_dictionary.h"

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE exitProcess(density_argonaut_encode_state *state, DENSITY_ARGONAUT_ENCODE_PROCESS process, DENSITY_KERNEL_ENCODE_STATE kernelEncodeState) {
    state->process = process;
    return kernelEncodeState;
}

DENSITY_FORCE_INLINE void density_argonaut_encode_push_to_output(density_memory_location *restrict out, uint16_t content) {
    *(uint16_t *) out->pointer = content;
    out->pointer += sizeof(uint16_t);
    out->available_bytes -= sizeof(uint16_t);
}

DENSITY_FORCE_INLINE void density_argonaut_encode_prepare_new_signature(density_memory_location *restrict out, density_argonaut_encode_state *restrict state) {
    state->shift = 0;
    state->signature = (density_argonaut_signature *) (out->pointer);
    *state->signature = 0;

    out->pointer += sizeof(density_argonaut_signature);
    out->available_bytes -= sizeof(density_argonaut_signature);
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE density_argonaut_encode_prepare_new_block(density_memory_location *restrict out, density_argonaut_encode_state *restrict state) {
    if (DENSITY_ARGONAUT_ENCODE_MINIMUM_OUTPUT_LOOKAHEAD > out->available_bytes)
        return DENSITY_KERNEL_ENCODE_STATE_STALL_ON_OUTPUT;

    switch (state->wordCount) {
        case DENSITY_ARGONAUT_PREFERRED_EFFICIENCY_CHECK_WORDS:
            if (state->efficiencyChecked ^ 0x1) {
                state->efficiencyChecked = 1;
                return DENSITY_KERNEL_ENCODE_STATE_INFO_EFFICIENCY_CHECK;
            }
            break;
        case DENSITY_ARGONAUT_PREFERRED_BLOCK_WORDS:
            state->wordCount = 0;
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

DENSITY_FORCE_INLINE void density_argonaut_encode_push_to_signature(density_memory_location *restrict out, density_argonaut_encode_state *state, uint64_t content, uint_fast8_t bits) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    *(state->signature) |= (content << state->shift);
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    *(state->signature) |= (content << ((56 - (state->shift & ~0x7)) + (state->shift & 0x7)));
#endif

    state->shift += bits;

    if (state->shift >= 0x40) {
        uint8_t remainder = (uint_fast8_t) (state->shift & 0x3f);
        density_argonaut_encode_prepare_new_signature(out, state);
        density_argonaut_encode_push_to_signature(out, state, content >> (bits - remainder), remainder); //todo check big endian
    }
}

DENSITY_FORCE_INLINE void density_argonaut_encode_push_to_signature_dual(density_memory_location *restrict out, density_argonaut_encode_state *state, uint32_t contentA, uint_fast8_t bitsA, uint32_t contentB, uint_fast8_t bitsB) {
    density_argonaut_encode_push_to_signature(out, state, (contentA << bitsB) | contentB, bitsA + bitsB);
}

DENSITY_FORCE_INLINE uint8_t density_argonaut_encode_fetch_form_rank_for_use(density_argonaut_encode_state *state, DENSITY_ARGONAUT_FORM form) {
    uint8_t rank = state->formStatistics[form].rank;

    state->formStatistics[form].usage++;
    if (rank) if (state->formStatistics[form].usage > state->formRanks[rank - 1].statistics->usage) {
        density_argonaut_form_statistics *replaced = state->formRanks[rank - 1].statistics;
        replaced->rank++;
        state->formRanks[rank - 1].statistics = &(state->formStatistics[form]);
        state->formRanks[rank].statistics = replaced;
        state->formStatistics[form].rank--;
    }

    return rank;
}

DENSITY_FORCE_INLINE void density_argonaut_encode_process_write_word(density_memory_location *restrict out, density_argonaut_encode_state *state) {
    // Check word predictions
    //if (state->lastLength <= 4) {
        if (state->dictionary.wordPredictions[state->lastByteHash].next_word.as_uint32_t == state->word.as_uint32_t) {
            uint8_t rank = density_argonaut_encode_fetch_form_rank_for_use(state, DENSITY_ARGONAUT_FORM_PREDICTIONS);
            density_argonaut_encode_push_to_signature(out, state, 0, rank + (uint8_t) 1);
            return;
        } else
            state->dictionary.wordPredictions[state->lastByteHash].next_word.as_uint32_t = state->word.as_uint32_t;
    //}

    // Word hashes
    uint32_t hash16 = (uint32_t) ((DENSITY_ARGONAUT_HASH32_MULTIPLIER * state->word.as_uint32_t) >> (32 - DENSITY_ARGONAUT_DICTIONARY_BITS));
    state->lastByteHash = hash16;//(uint8_t) (hash16 ^ (hash16 >> 8));

    // *Dictionary search
    uint8_t rank;
    density_argonaut_dictionary_word_entry *wordFound = &state->dictionary.wordsA[hash16];
    bool alternateDictionary = false;
    if (wordFound->word.as_uint32_t == state->word.as_uint32_t)
        goto word_found;
    wordFound = &state->dictionary.wordsB[hash16];
    alternateDictionary = true;
    if (wordFound->word.as_uint32_t == state->word.as_uint32_t) {
        word_found:
        /*wordFound->usage++;
        wordFound->durability++;

        // Update usage and rank
        if (wordFound->rank) {
            uint_fast8_t lowerRank = wordFound->rank;
            uint_fast8_t upperRank = lowerRank - (uint_fast8_t) 1;
            if (state->dictionary.wordRanks[upperRank] == NULL) {
                wordFound->rank = upperRank;
                state->dictionary.wordRanks[upperRank] = wordFound;
                state->dictionary.wordRanks[lowerRank] = NULL;
            } else if (wordFound->usage > state->dictionary.wordRanks[upperRank]->usage) {
                density_argonaut_dictionary_word_entry *replaced = state->dictionary.wordRanks[upperRank];
                wordFound->rank = upperRank;
                state->dictionary.wordRanks[upperRank] = wordFound;
                replaced->rank = lowerRank;
                state->dictionary.wordRanks[lowerRank] = replaced;
            }
        }

        // Check if we have an 8 bit rank
        if (wordFound->rank < 0xff) {
            uint8_t rank = density_argonaut_encode_fetch_form_rank_for_use(state, DENSITY_ARGONAUT_FORM_WORD_RANK);
            density_argonaut_encode_push_to_signature_dual(out, state, 0, rank + (uint8_t) 1, 0, density_argonaut_huffman_codes[wordFound->rank].bitSize);
            return;
        }*/

        // Write dictionary hash value
        rank = density_argonaut_encode_fetch_form_rank_for_use(state, DENSITY_ARGONAUT_FORM_DICTIONARY);
        density_argonaut_encode_push_to_signature(out, state, 0x1, (uint8_t) (DENSITY_ARGONAUT_DICTIONARY_BITS & 0x7) + rank + (uint8_t) 1);
        //density_argonaut_encode_push_to_output(out, hash16);
        *(uint32_t *) out->pointer = 0x1234lu;
        out->available_bytes -= (DENSITY_ARGONAUT_DICTIONARY_BITS >> 3);
    } else {
        // Assign new word values
        wordFound->word.as_uint32_t = state->word.as_uint32_t;
        wordFound->rank = 0xff;
        wordFound->usage = 1;
        wordFound->durability = 0;

        // Swap entries if origin is the alternate dictionary
        if (alternateDictionary) {
            density_argonaut_dictionary_word_entry replaced = state->dictionary.wordsA[hash16];
            state->dictionary.wordsA[hash16] = *wordFound;
            state->dictionary.wordsB[hash16] = replaced;
        }

        rank = density_argonaut_encode_fetch_form_rank_for_use(state, DENSITY_ARGONAUT_FORM_ENCODED);
        density_argonaut_encode_push_to_signature(out, state, 0x1, rank + (uint8_t) 1);
        *(uint32_t *) out->pointer = 0x5454lu;
        out->available_bytes -= 4;
    }

    return;

    // Double dictionary search
    /*bool alternateDictionary = false;
    density_argonaut_dictionary_word_entry *wordFound = &state->dictionary.wordsA[hash16];
    if (wordFound->word.as_uint64_t == state->word.as_uint64_t)
        goto word_found;
    wordFound = &state->dictionary.wordsB[hash16];
    alternateDictionary = true;
    if (wordFound->word.as_uint64_t == state->word.as_uint64_t) {
        word_found:
        wordFound->usage++;
        wordFound->durability++;

        // Update usage and rank
        if (wordFound->rank) {
            uint_fast8_t lowerRank = wordFound->rank;
            uint_fast8_t upperRank = lowerRank - (uint_fast8_t) 1;
            if (state->dictionary.wordRanks[upperRank] == NULL) {
                wordFound->rank = upperRank;
                state->dictionary.wordRanks[upperRank] = wordFound;
                state->dictionary.wordRanks[lowerRank] = NULL;
            } else if (wordFound->usage > state->dictionary.wordRanks[upperRank]->usage) {
                density_argonaut_dictionary_word_entry *replaced = state->dictionary.wordRanks[upperRank];
                wordFound->rank = upperRank;
                state->dictionary.wordRanks[upperRank] = wordFound;
                replaced->rank = lowerRank;
                state->dictionary.wordRanks[lowerRank] = replaced;
            }
        }

        // Check if we have an 8 bit rank
        if (wordFound->rank < 0xff) {
            uint8_t rank = density_argonaut_encode_fetch_form_rank_for_use(state, DENSITY_ARGONAUT_FORM_WORD_RANK);
            density_argonaut_encode_push_to_signature_dual(out, state, 0, rank + (uint8_t) 1, 0, density_argonaut_huffman_codes[wordFound->rank].bitSize);
            return;
        }

        // Write dictionary hash value
        uint8_t rank = density_argonaut_encode_fetch_form_rank_for_use(state, DENSITY_ARGONAUT_FORM_DICTIONARY);
        density_argonaut_encode_push_to_signature_dual(out, state, 0, rank + (uint8_t) 1, (uint8_t) alternateDictionary, 1);
        density_argonaut_encode_push_to_output(out, hash16);
    } else {
        if (wordFound->durability)
            wordFound->durability--;
        else {
            // Remove previous word from rank table
            state->dictionary.wordRanks[wordFound->rank] = NULL;

            // Assign new word values
            wordFound->word.as_uint64_t = state->word.as_uint64_t;
            wordFound->rank = 0xff;
            wordFound->usage = 1;
            wordFound->durability = 0;

            // Swap entries if origin is the alternate dictionary
            if (alternateDictionary) {
                density_argonaut_dictionary_word_entry replaced = state->dictionary.wordsA[hash16];
                state->dictionary.wordsA[hash16] = *wordFound;
                state->dictionary.wordsB[hash16] = replaced;
            }
        }

        // Word is not in a dictionary, it has to be encoded
        uint8_t rank = density_argonaut_encode_fetch_form_rank_for_use(state, DENSITY_ARGONAUT_FORM_ENCODED);
        density_argonaut_encode_push_to_signature(out, state, 0, rank + (uint8_t) 1);

        // Look for word inner predictions
        density_argonaut_encode_push_to_signature(out, state, 0, density_argonaut_huffman_codes[state->dictionary.letters[state->word.letters[0]].rank].bitSize);
        for (int wordLetter = 1; wordLetter < state->word.length; wordLetter++) {
            if (wordLetter == 5 && state->dictionary.fourgramPredictions[(((uint32_t) (state->word.as_uint64_t & 0xFFFFFFFF)) * DENSITY_ARGONAUT_HASH32_MULTIPLIER) >> 16].next_part == (uint32_t) ((state->word.as_uint64_t & 0xFFFFFFFF00000000) >> 32)) {
                density_argonaut_encode_push_to_signature(out, state, 0x3, 2);
                goto exit_inner_predictions;
            } else if (wordLetter >= 2 && wordLetter < 4 && state->word.length >= wordLetter + 2 && state->dictionary.bigramPredictions[((uint16_t) (state->word.letters[wordLetter - 2]) << 8) + (uint16_t) state->word.letters[wordLetter - 1]].next_bigram == (uint16_t) (state->word.letters[wordLetter] << 8) + state->word.letters[wordLetter + 1]) {
                density_argonaut_encode_push_to_signature(out, state, 0x3, 2);
                wordLetter += 1;
            } else if (state->dictionary.unigramPredictions[(uint8_t) state->word.letters[wordLetter - 1]].next_letter == state->word.letters[wordLetter]) {
                density_argonaut_encode_push_to_signature(out, state, 0x2, 2);
            } else if (wordLetter >= 2 && state->dictionary.bigramPredictions[((uint16_t) (state->word.letters[wordLetter - 2]) << 8) + (uint16_t) state->word.letters[wordLetter - 1]].next_letter == state->word.letters[wordLetter]) {
                density_argonaut_encode_push_to_signature(out, state, 0x1, 2);
            } else if (wordLetter > 5 && state->dictionary.fourgramPredictions[(((uint32_t) (((state->word.as_uint64_t) >> (wordLetter - 4)) & 0xFFFFFFFF)) * DENSITY_ARGONAUT_HASH32_MULTIPLIER) >> 16].next_letter == state->word.letters[wordLetter]) {
                density_argonaut_encode_push_to_signature(out, state, 0x3, 2);
            } else {
                density_argonaut_encode_push_to_signature_dual(out, state, 0x0, 2, 0, density_argonaut_huffman_codes[state->dictionary.letters[state->word.letters[wordLetter]].rank].bitSize);
            }
        }
        exit_inner_predictions:

        // Update word inner predictions
        if (state->word.length > 1)
            for (uint_fast8_t wordLetter = 1; wordLetter < state->word.length; wordLetter++)
                state->dictionary.unigramPredictions[(uint8_t) state->word.letters[wordLetter - 1]].next_letter = (uint8_t) state->word.letters[wordLetter];

        if (state->word.length > 2)
            for (uint_fast8_t wordLetter = 2; wordLetter < state->word.length; wordLetter++)
                state->dictionary.bigramPredictions[((uint16_t) (state->word.letters[wordLetter - 2]) << 8) + (uint16_t) state->word.letters[wordLetter - 1]].next_letter = (uint8_t) state->word.letters[wordLetter];

        if (state->word.length > 3)
            for (uint_fast8_t wordLetter = 3; wordLetter < state->word.length; wordLetter++)
                state->dictionary.bigramPredictions[((uint16_t) (state->word.letters[wordLetter - 3]) << 8) + (uint16_t) state->word.letters[wordLetter - 2]].next_bigram = (uint16_t) (state->word.letters[wordLetter - 1] << 8) + state->word.letters[wordLetter];

        if (state->word.length > 4) {
            for (uint_fast8_t wordLetter = 4; wordLetter < state->word.length; wordLetter++)
                state->dictionary.fourgramPredictions[(((uint32_t) (((state->word.as_uint64_t) >> (wordLetter - 4)) & 0xFFFFFFFF)) * DENSITY_ARGONAUT_HASH32_MULTIPLIER) >> 16].next_letter = (uint8_t) state->word.letters[wordLetter];

            state->dictionary.fourgramPredictions[((uint32_t) (state->word.as_uint64_t & 0xFFFFFFFF) * DENSITY_ARGONAUT_HASH32_MULTIPLIER) >> 16].next_part = (uint32_t) ((state->word.as_uint64_t & 0xFFFFFFFF00000000) >> 32);
        }
    }*/
}

DENSITY_FORCE_INLINE void density_argonaut_encode_mask_word(density_argonaut_encode_state *restrict state) {
    if (state->word.length < 8)
        state->word.as_uint32_t &= (0xFFFFFFFFllu >> (bitsizeof(density_argonaut_word) - state->word.length * bitsizeof(uint8_t)));
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

DENSITY_FORCE_INLINE uint_fast8_t density_argonaut_encode_find_position32(uint32_t chain) {
    if (chain)
        return 1 + (__builtin_ctz(chain) >> 3);
    else
        return (uint_fast8_t) sizeof(uint32_t);
}

DENSITY_FORCE_INLINE uint_fast8_t density_argonaut_encode_find_position(uint64_t chain) {
    if (chain) {
        uint_fast32_t word_beginning = (uint_fast32_t) (chain & 0xFFFFFFFF);
        if (word_beginning)
            return 1 + (__builtin_ctz(word_beginning) >> 3);
        else {
            uint_fast32_t word_end = (uint_fast32_t) (chain >> 32);
            return 5 + (__builtin_ctz(word_end) >> 3);
        }
    } else
        return (uint_fast8_t) sizeof(uint64_t);
}

DENSITY_FORCE_INLINE uint_fast8_t density_argonaut_encode_find_first_separator_position(uint64_t *restrict chain, uint8_t separator) {
    return density_argonaut_encode_find_position(density_argonaut_contains_value(*chain, separator));
}

DENSITY_FORCE_INLINE uint_fast8_t density_argonaut_encode_find_first_separator_position32(uint32_t *restrict chain, uint8_t separator) {
    return density_argonaut_encode_find_position32(density_argonaut_contains_value32(*chain, separator));
}

DENSITY_FORCE_INLINE uint_fast8_t density_argonaut_encode_read_separators_until_letter_limited(density_memory_location *restrict in, uint8_t separator, uint_fast64_t limit) {
    uint_fast64_t start = in->available_bytes;
    while (limit--) {
        in->pointer++;
        in->available_bytes--;
        if (*(in->pointer) != separator)
            return (uint_fast8_t) (start - in->available_bytes);
    }
    return (uint_fast8_t) (start - in->available_bytes);
}

DENSITY_FORCE_INLINE bool density_argonaut_encode_read_word(density_argonaut_encode_state *restrict state) {
    density_memory_location *in = state->readMemoryLocation;
    uint_fast8_t startLength = state->word.length;
    uint8_t *letter = in->pointer;
    uint8_t separator = (*state->dictionary.letterRanks)->letter;
    bool startWithSeparator;

    if (startLength)
        goto step_by_step_read;

    if (in->available_bytes & 0xFFFFFFFFFFFFFFFCllu) {
        state->word.as_uint32_t = *(uint32_t *) letter;
        if (density_likely(*letter != separator)) {
            state->word.length = density_argonaut_encode_find_first_separator_position32(&state->word.as_uint32_t, separator);
            in->pointer += state->word.length;
            in->available_bytes -= state->word.length;
        } else
            state->word.length = density_argonaut_encode_read_separators_until_letter_limited(in, separator, DENSITY_ARGONAUT_DICTIONARY_MAX_WORD_LETTERS);
        goto update_letter_stats;
    }

    step_by_step_read:
    startWithSeparator = ((startLength ? *state->word.letters : *letter) == separator);

    while (in->available_bytes) {
        density_argonaut_dictionary_letter_entry *letterEntry = &state->dictionary.letters[*letter];

        // Stop if the word started with the separator letter and a different letter has been read
        if (startWithSeparator && letterEntry->rank)
            goto update_letter_stats;

        // Store and read new letter
        state->word.letters[state->word.length++] = *letter;
        letter = ++in->pointer;
        in->available_bytes--;

        // Stop if word length has reached the maximum of 8 letters
        if (state->word.length == DENSITY_ARGONAUT_DICTIONARY_MAX_WORD_LETTERS)
            goto update_letter_stats;

        // Stop if we found the separator and the word didn't begin with a separator
        if (!startWithSeparator && !letterEntry->rank)
            goto update_letter_stats;
    }
    return true;

    // Update letter usage and rank
    update_letter_stats:
    if (!(state->wordCount % 5)) {
        density_argonaut_encode_update_letter_rank(&state->dictionary.letters[*state->word.letters], state);
        if (state->word.length > 2) {
            density_argonaut_encode_update_letter_rank(&state->dictionary.letters[*(state->word.letters + 2)], state);
            if (state->word.length > 4) {
                density_argonaut_encode_update_letter_rank(&state->dictionary.letters[*(state->word.letters + 4)], state);
                if (state->word.length > 6)
                    density_argonaut_encode_update_letter_rank(&state->dictionary.letters[*(state->word.letters + 6)], state);
            }
        }
    }

    // Masking
    density_argonaut_encode_mask_word(state);

    return false;
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE density_argonaut_encode_init(density_argonaut_encode_state *state) {
    state->word.length = 0;
    state->wordCount = 0;
    state->efficiencyChecked = 0;
    density_argonaut_dictionary_reset(&state->dictionary);

#if DENSITY_ENABLE_PARALLELIZABLE_DECOMPRESSIBLE_OUTPUT == DENSITY_YES
    state->resetCycle = DENSITY_DICTIONARY_PREFERRED_RESET_CYCLE - 1;
#endif

    state->formStatistics[DENSITY_ARGONAUT_FORM_PREDICTIONS].usage = 0;
    state->formStatistics[DENSITY_ARGONAUT_FORM_PREDICTIONS].rank = 3;
    state->formStatistics[DENSITY_ARGONAUT_FORM_WORD_RANK].usage = 0;
    state->formStatistics[DENSITY_ARGONAUT_FORM_WORD_RANK].rank = 1;
    state->formStatistics[DENSITY_ARGONAUT_FORM_DICTIONARY].usage = 0;
    state->formStatistics[DENSITY_ARGONAUT_FORM_DICTIONARY].rank = 0;
    state->formStatistics[DENSITY_ARGONAUT_FORM_ENCODED].usage = 0;
    state->formStatistics[DENSITY_ARGONAUT_FORM_ENCODED].rank = 2;

    state->formRanks[0].statistics = &state->formStatistics[DENSITY_ARGONAUT_FORM_DICTIONARY];
    state->formRanks[1].statistics = &state->formStatistics[DENSITY_ARGONAUT_FORM_WORD_RANK];
    state->formRanks[2].statistics = &state->formStatistics[DENSITY_ARGONAUT_FORM_ENCODED];
    state->formRanks[3].statistics = &state->formStatistics[DENSITY_ARGONAUT_FORM_PREDICTIONS];

    uint_fast8_t count = 0xff;
    do {
        state->dictionary.letters[count].letter = count;
        state->dictionary.letters[count].rank = count;
        state->dictionary.letterRanks[count] = &state->dictionary.letters[count];
    } while (count--);

    uint_fast32_t dCount = (1 << DENSITY_ARGONAUT_DICTIONARY_BITS) - 1;
    do {
        state->dictionary.wordsA[dCount].word.as_uint32_t = 0;
        state->dictionary.wordsA[dCount].usage = 0;
        state->dictionary.wordsA[dCount].rank = 0xff;
        state->dictionary.wordsB[dCount].word.as_uint32_t = 0;
        state->dictionary.wordsB[dCount].usage = 0;
        state->dictionary.wordsB[dCount].rank = 0xff;
    } while (dCount--);

    count = 0xff;
    do {
        state->dictionary.wordRanks[count] = NULL;
    } while (count--);

    return exitProcess(state, DENSITY_ARGONAUT_ENCODE_PROCESS_PREPARE_NEW_BLOCK, DENSITY_KERNEL_ENCODE_STATE_READY);
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE density_argonaut_encode_continue(density_memory_teleport *restrict in, density_memory_location *restrict out, density_argonaut_encode_state *restrict state) {
    DENSITY_KERNEL_ENCODE_STATE returnState;

    // Dispatch
    switch (state->process) {
        case DENSITY_ARGONAUT_ENCODE_PROCESS_PREPARE_NEW_BLOCK:
            goto prepare_new_block;
        case DENSITY_ARGONAUT_ENCODE_PROCESS_PREPARE_INPUT:
            goto prepare_input;
        case DENSITY_ARGONAUT_ENCODE_PROCESS_PREPARE_OUTPUT:
            goto prepare_output;
        default:
            return DENSITY_KERNEL_ENCODE_STATE_ERROR;
    }

    // Prepare new block
    prepare_new_block:
    if ((returnState = density_argonaut_encode_prepare_new_block(out, state)))
        return exitProcess(state, DENSITY_ARGONAUT_ENCODE_PROCESS_PREPARE_NEW_BLOCK, returnState);

    // Prepare input
    prepare_input:
    if (!(state->readMemoryLocation = density_memory_teleport_read(in, 2048)))
        return exitProcess(state, DENSITY_ARGONAUT_ENCODE_PROCESS_PREPARE_INPUT, DENSITY_KERNEL_ENCODE_STATE_STALL_ON_INPUT);

    // Main loop
    while (true) {
        //if (density_argonaut_encode_read_word(state))
        //    goto prepare_input;
        state->word.as_uint32_t = *(uint32_t*)state->readMemoryLocation->pointer;
        state->word.length = 4;
        state->readMemoryLocation->pointer+=4;
        state->readMemoryLocation->available_bytes-=4;
        if(!state->readMemoryLocation->available_bytes)
            goto prepare_input;

        prepare_output:
        if (out->available_bytes < 16)
            return exitProcess(state, DENSITY_ARGONAUT_ENCODE_PROCESS_PREPARE_OUTPUT, DENSITY_KERNEL_ENCODE_STATE_STALL_ON_OUTPUT);

        density_argonaut_encode_process_write_word(out, state);
        state->wordCount++;
        state->lastLength = state->word.length;
        state->word.length = 0;
    }
}

DENSITY_FORCE_INLINE DENSITY_KERNEL_ENCODE_STATE density_argonaut_encode_finish(density_memory_teleport *restrict in, density_memory_location *restrict out, density_argonaut_encode_state *restrict state) {
    DENSITY_KERNEL_ENCODE_STATE returnState;

    // Dispatch
    switch (state->process) {
        case DENSITY_ARGONAUT_ENCODE_PROCESS_PREPARE_NEW_BLOCK:
            goto prepare_new_block;
        case DENSITY_ARGONAUT_ENCODE_PROCESS_PREPARE_INPUT:
            goto prepare_input;
        case DENSITY_ARGONAUT_ENCODE_PROCESS_PREPARE_OUTPUT:
            goto prepare_output;
        default:
            return DENSITY_KERNEL_ENCODE_STATE_ERROR;
    }

    // Prepare new block
    prepare_new_block:
    if ((returnState = density_argonaut_encode_prepare_new_block(out, state)))
        return exitProcess(state, DENSITY_ARGONAUT_ENCODE_PROCESS_PREPARE_NEW_BLOCK, returnState);

    // Prepare input
    prepare_input:
    if (!(state->readMemoryLocation = density_memory_teleport_read(in, 256)))
        state->readMemoryLocation = density_memory_teleport_read(in, density_memory_teleport_available(in));

    // Main loop
    while (true) {
        if (density_argonaut_encode_read_word(state))
            goto finish;

        prepare_output:
        if (out->available_bytes < 16)
            return exitProcess(state, DENSITY_ARGONAUT_ENCODE_PROCESS_PREPARE_OUTPUT, DENSITY_KERNEL_ENCODE_STATE_STALL_ON_OUTPUT);

        density_argonaut_encode_process_write_word(out, state);
        state->wordCount++;
        state->lastLength = state->word.length;
        state->word.length = 0;
    }

    finish:

    density_argonaut_encode_mask_word(state);

    // todo check output size
    density_argonaut_encode_process_write_word(out, state);
    state->wordCount++;

    /*uint_fast64_t available = density_memory_teleport_available(in);
    state->readMemoryLocation = density_memory_teleport_read(in, available);
    state->readMemoryLocation->pointer += available;
    state->readMemoryLocation->available_bytes -= available;*/

    return DENSITY_KERNEL_ENCODE_STATE_READY;
}