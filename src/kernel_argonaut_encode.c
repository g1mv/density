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
#include "memory_location.h"

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
    density_argonaut_form_statistics *stats = &state->formStatistics[form];

    const uint8_t rank = stats->rank;
    if (density_unlikely(rank)) {
        //if (!(stats->usage & 0xFF)) {
        density_argonaut_form_rank *rankCurrent = &state->formRanks[rank];
        density_argonaut_form_rank *rankUpper = &state->formRanks[rank - 1];
        if (density_unlikely(stats->usage > rankUpper->statistics->usage)) {
            density_argonaut_form_statistics *replaced = rankUpper->statistics;
            replaced->rank++;
            stats->rank--;
            rankUpper->statistics = stats;
            rankCurrent->statistics = replaced;
        }
        //}
        stats->usage++;
        return density_argonaut_form_codes[rank];
    } else {
        stats->usage++;
        return density_argonaut_form_codes[0];
    }
}

DENSITY_FORCE_INLINE void density_argonaut_encode_update_letter_rank(density_argonaut_dictionary_letter_entry *restrict letterEntry, density_argonaut_encode_state *restrict state) {
    letterEntry->usage++;
    if (!(letterEntry->usage & 0xF)) {  // Check once every 16 times
        if (letterEntry->rank) {
            uint8_t lowerRank = letterEntry->rank;
            uint8_t upperRank = lowerRank - (uint8_t) 1;
            if (density_unlikely(letterEntry->usage > state->dictionary.letterRanks[upperRank]->usage)) {
                density_argonaut_dictionary_letter_entry *swappedLetterEntry = state->dictionary.letterRanks[upperRank];
                letterEntry->rank = upperRank;
                state->dictionary.letterRanks[upperRank] = letterEntry;
                swappedLetterEntry->rank = lowerRank;
                state->dictionary.letterRanks[lowerRank] = swappedLetterEntry;
            }
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

//int i = 0;

DENSITY_FORCE_INLINE uint32_t density_argonaut_encode_mask_word(uint32_t word, uint8_t length) {
    if (length < 4)
        return word & (0xFFFFFFFFlu >> (bitsizeof(uint32_t) - length * bitsizeof(uint8_t)));
    else
        return word;
}

DENSITY_FORCE_INLINE uint32_t density_argonaut_hash(uint32_t chunk, uint8_t length) {
    uint32_t hashw = (uint32_t) (chunk & 0xFF);
    hashw += (uint32_t) (chunk % ((1 << DICT_BITS) - 256));
    printf("%lu,", hashw);
    return hashw;
}


///* INTERFACE TO THE MODEL. */
//
//
///* THE SET OF SYMBOLS THAT MAY BE ENCODED. */
//
//#define No_of_chars 256            /* Number of character symbols      */
//#define EOF_symbol (No_of_chars+1)    /* Index of EOF symbol              */
//
//#define No_of_symbols (No_of_chars+1)    /* Total number of symbols          */
//
//
///* TRANSLATION TABLES BETWEEN CHARACTERS AND SYMBOL INDEXES. */
//
//int char_to_index[No_of_chars];
///* To index from character          */
//unsigned char index_to_char[No_of_symbols + 1]; /* To character from index    */
//
//
///* CUMULATIVE FREQUENCY TABLE. */
//
//#define Max_frequency 16383        /* Maximum allowed frequency count  */
//
//int cum_freq[No_of_symbols + 1];
///* Cumulative symbol frequencies    */
//
//int freq[No_of_symbols + 1];    /* Symbol frequencies                       */
//
//
///* INITIALIZE THE MODEL. */
//
//DENSITY_FORCE_INLINE void start_model() {
//    int i;
//    for (i = 0; i < No_of_chars; i++) {        /* Set up tables that       */
//        char_to_index[i] = i + 1;            /* translate between symbol */
//        index_to_char[i + 1] = i;            /* indexes and characters.  */
//    }
//    for (i = 0; i <= No_of_symbols; i++) {    /* Set up initial frequency */
//        freq[i] = 1;                /* counts to be one for all */
//        cum_freq[i] = No_of_symbols - i;        /* symbols.                 */
//    }
//    freq[0] = 0;                /* Freq[0] must not be the  */
//}                        /* same as freq[1].         */
//
//
///* UPDATE THE MODEL TO ACCOUNT FOR A NEW SYMBOL. */
//
//DENSITY_FORCE_INLINE void update_model(symbol)
//        int symbol;            /* Index of new symbol                      */
//{
//    int i;            /* New index for symbol                     */
//    if (cum_freq[0] == Max_frequency) {        /* See if frequency counts  */
//        int cum;                /* are at their maximum.    */
//        cum = 0;
//        for (i = No_of_symbols; i >= 0; i--) {    /* If so, halve all the     */
//            freq[i] = (freq[i] + 1) / 2;        /* counts (keeping them     */
//            cum_freq[i] = cum;            /* non-zero).               */
//            cum += freq[i];
//        }
//    }
//    for (i = symbol; freq[i] == freq[i - 1]; i--);    /* Find symbol's new index. */
//    if (i < symbol) {
//        int ch_i, ch_symbol;
//        ch_i = index_to_char[i];        /* Update the translation   */
//        ch_symbol = index_to_char[symbol];    /* tables if the symbol has */
//        index_to_char[i] = ch_symbol;           /* moved.                   */
//        index_to_char[symbol] = ch_i;
//        char_to_index[ch_i] = symbol;
//        char_to_index[ch_symbol] = i;
//    }
//    freq[i] += 1;                /* Increment the frequency  */
//    while (i > 0) {                /* count for the symbol and */
//        i -= 1;                    /* update the cumulative    */
//        cum_freq[i] += 1;            /* frequencies.             */
//    }
//}
//
///* DECLARATIONS USED FOR ARITHMETIC ENCODING AND DECODING */
//
//
///* SIZE OF ARITHMETIC CODE VALUES. */
//
//#define Code_value_bits 16        /* Number of bits in a code value   */
//typedef long code_value;        /* Type of an arithmetic code value */
//
//#define Top_value (((long)1<<Code_value_bits)-1)      /* Largest code value */
//
//
///* HALF AND QUARTER POINTS IN THE CODE VALUE RANGE. */
//
//#define First_qtr (Top_value/4+1)    /* Point after first quarter        */
//#define Half      (2*First_qtr)        /* Point after first half           */
//#define Third_qtr (3*First_qtr)        /* Point after third quarter        */
//
///* ARITHMETIC ENCODING ALGORITHM. */
//
//static void bit_plus_follow();    /* Routine that follows                     */
//
//
///* CURRENT STATE OF THE ENCODING. */
//
//static code_value low, high;
///* Ends of the current code region          */
//static long bits_to_follow;    /* Number of opposite bits to output after  */
///* the next bit.                            */
//
//
///* START ENCODING A STREAM OF SYMBOLS. */
//
//start_encoding() {
//    low = 0;                    /* Full code range.         */
//    high = Top_value;
//    bits_to_follow = 0;                /* No bits to follow next.  */
//}
//
//
///* ENCODE A SYMBOL. */
//
//DENSITY_FORCE_INLINE void encode_symbol(density_memory_location *restrict out, density_argonaut_encode_state *restrict state, int symbol, int cum_freq[])
//{
//    long range;            /* Size of the current code region          */
//    range = (long) (high - low) + 1;
//    high = low +                /* Narrow the code region   */
//            (range * cum_freq[symbol - 1]) / cum_freq[0] - 1;    /* to that allotted to this */
//    low = low +                /* symbol.                  */
//            (range * cum_freq[symbol]) / cum_freq[0];
//    for (; ;) {                    /* Loop to output bits.     */
//        if (high < Half) {
//            bit_plus_follow(out, state, 0);            /* Output 0 if in low half. */
//        }
//        else if (low >= Half) {            /* Output 1 if in high half.*/
//            bit_plus_follow(out, state, 1);
//            low -= Half;
//            high -= Half;            /* Subtract offset to top.  */
//        }
//        else if (low >= First_qtr            /* Output an opposite bit   */
//                && high < Third_qtr) {        /* later if in middle half. */
//            bits_to_follow += 1;
//            low -= First_qtr;            /* Subtract offset to middle*/
//            high -= First_qtr;
//        }
//        else break;                /* Otherwise exit loop.     */
//        low = 2 * low;
//        high = 2 * high + 1;            /* Scale up code range.     */
//    }
//}
//
//
///* FINISH ENCODING THE STREAM. */
//
//DENSITY_FORCE_INLINE void done_encoding(density_memory_location *restrict out, density_argonaut_encode_state *restrict state) {
//    bits_to_follow += 1;            /* Output two bits that     */
//    if (low < First_qtr) bit_plus_follow(0);    /* select the quarter that  */
//    else bit_plus_follow(out, state, 1);            /* the current code range   */
//}                        /* contains.                */
//
//
///* OUTPUT BITS PLUS FOLLOWING OPPOSITE BITS. */
//
//DENSITY_FORCE_INLINE void bit_plus_follow(density_memory_location *restrict out, density_argonaut_encode_state *restrict state, int bit)
//{
//    density_argonaut_encode_push_to_signature(out, state, bit, 1);                /* Output the bit.          */
//    //output_bit(bit);
//    while (bits_to_follow > 0) {
//        density_argonaut_encode_push_to_signature(out, state, !bit, 1);
//        //output_bit(!bit);            /* Output bits_to_follow    */
//        bits_to_follow -= 1;            /* opposite bits. Set       */
//    }                        /* bits_to_follow to zero.  */
//}

#define H_MAX 256

typedef struct listnode *listp;
typedef struct listnode {
    int number;
    /* used by Algorithm FGK and Vitter. */
    listp next;
    /* for list processing convenience.  */
    unsigned long freq;
    signed int ch;
    /* signed for handling symbols <= -1 */
    listp parent, child_1, child_2;
} listnode_t;

typedef struct {
    unsigned char b;
    unsigned long f;
} hfreq_type;

extern unsigned int hcount;
extern hfreq_type hfreq[];

extern listnode_t *list,    /* the list (current) pointer  */
        *top,                   /* the top of the Huffman tree */
        *list_head;             /* the list_head pointer       */

/* this is the list of symbol addresses in the binary/Huffman tree .*/
extern listp hufflist[];

extern listnode_t hnodes[];
extern unsigned int hn;

/*
the following are conveniently used
by the adaptive Huffman functions
*/
extern unsigned hmax_symbols;
extern int hmin;
extern unsigned int hsymbol_bit_size;

/* forward declarations */
listnode_t *create_node(void);

void insert(listnode_t *node);

void init_hufflist(void);

void init_hfreq(void);

void create_symbol_list(void);

listnode_t *get_least_node(void);

void create_hufftree(void);

unsigned int hcount = 0;
hfreq_type hfreq[H_MAX];

listnode_t *list = NULL,   /* the list (current) pointer  */
        *top = NULL,           /* the top of the Huffman tree */
        *list_head = NULL;     /* the list_head pointer       */

/* this is the list of symbol addresses in the binary/Huffman tree .*/
listp hufflist[H_MAX];   /* just an array of pointers.  */

/*
Static allocation is faster;
extra two nodes used by Dynamic Huffman (Algorithm FGK & Vitter).
*/
listnode_t hnodes[H_MAX * 2 + 1];
/* the buffer of Huffman nodes. */
unsigned int hn = 0;                /* this buffer's counter. */

/*
the following are conveniently used
by the adaptive Huffman functions
*/
unsigned hmax_symbols = H_MAX;
int hmin = -1;
/* default: log_2(H_MAX) = 8 */
unsigned int hsymbol_bit_size = 8;
bool first = true;

listnode_t *create_node(void) {
    listnode_t *node;

    if (hn >= (H_MAX * 2 + 1)) return NULL;
    node = &hnodes[hn++];   /* get an address. */

    node->number = 0;
    node->freq = (unsigned long) 0;
    node->next = NULL;

    node->parent = NULL;
    node->child_1 = NULL;
    node->child_2 = NULL;
    node->ch = -1;

    return node;            /* pass the address. */
}

/* inserts the node to list */
void insert(listnode_t *node) {
    listnode_t *curr, *prev = NULL;

    curr = list;
    if (list) {
        while (curr && node->freq > curr->freq) {
            prev = curr;
            curr = curr->next;
        }

        /* when node->freq is < and becomes the start of list */
        if (prev == NULL && curr) list = node;
        else prev->next = node;

        node->next = curr;
    }
    else {
        if (list_head == NULL) list_head = node;
        list = node;
    }
}

void init_hufflist(void) {
    int i = 0;

    for (; i < hmax_symbols; ++i) {
        hufflist[i] = NULL;
    }
}

#define ZERO_NODE_SYMBOL  -2
#define INTERNAL_NODE     -1
#define ROOT_NODE_NUMBER  (H_MAX*2)

#define huffnode_t listnode_t

int hc;                        /* the "current" symbol. */
huffnode_t *zero_node = NULL;  /* the 0-node. */

/*
this array contains the node addresses
as indexed by their node numbers.
*/
huffnode_t *numbers[H_MAX * 2 + 1];

/* the counter of numbers assigned so far. */
int aNUMBER = ROOT_NODE_NUMBER;

void create_new_zero_node(int c);

void swap_nodes(huffnode_t *a, huffnode_t *b);

/*
This function creates a new zero node from a current zero
node by creating two children nodes, one of which becomes
the new zero node.

The function accepts the new symbol and makes it the second
child (or "right" child) of the current zero node. It is the
first child (or "left" child) which becomes the new zero node.
*/
void create_new_zero_node(int c) {
    /* create children and assign links. */
    zero_node->child_1 = create_node();
    zero_node->child_2 = create_node();

    zero_node->child_1->parent = zero_node;
    zero_node->child_2->parent = zero_node;

    /* now an internal node, reset. */
    zero_node->ch = INTERNAL_NODE;

    /* new symbol's node is child_2 or "right." */
    hufflist[c] = zero_node->child_2;
    hufflist[c]->ch = c;

    /* Number the nodes. */
    zero_node->number = aNUMBER--;
    hufflist[c]->number = aNUMBER--;

    /*
        Store them in the hash array indexed by
        the node numbers. Perfect hash again.
    */
    numbers[zero_node->number] = zero_node;
    numbers[hufflist[c]->number] = hufflist[c];

    /* new zero node is child_1 or "left." */
    zero_node = zero_node->child_1;
    zero_node->ch = ZERO_NODE_SYMBOL;
}

/*
This function swaps two nodes by "re-pointing" the two
nodes' parent pointers, as well as the parent's children
pointers. Note that it is very careful when the two nodes
are siblings (same parent) and just re-arranges the parent's
children pointers.
*/
void swap_nodes(huffnode_t *a, huffnode_t *b) {
    huffnode_t *parent, *temp = NULL;
    int i = 0;

    /* special swap if they have the same parent. */
    if (a->parent == b->parent) {
        parent = a->parent;
        if (parent->child_1 == a) {
            parent->child_1 = b;
            parent->child_2 = a;
        }
        else {
            parent->child_1 = a;
            parent->child_2 = b;
        }

        goto numbers_stuff;
    }

    if (a->parent->child_1 == a) {
        a->parent->child_1 = b;
    }
    else {
        a->parent->child_2 = b;
    }

    if (b->parent->child_1 == b) {
        b->parent->child_1 = a;
    }
    else {
        b->parent->child_2 = a;
    }

    parent = a->parent;
    a->parent = b->parent;
    b->parent = parent;

    numbers_stuff:

    /* Swap nodes in numbers[] array and their numbers. */
    temp = numbers[a->number];
    numbers[a->number] = numbers[b->number];
    numbers[b->number] = temp;

    i = a->number;
    a->number = b->number;
    b->number = i;
}

void init_hfreq(void) {
    int i = 0;

    for (; i < hmax_symbols; ++i) {
        hfreq[i].f = (unsigned long) 0;
        hfreq[i].b = (unsigned char) i;
    }
}

void create_symbol_list(void) {
    unsigned int i = 0;
    listnode_t *node;

    /* creates the initial *SORTED* list of symbols. */
    hcount = 0;
    for (; i < hmax_symbols; ++i) {
        if (hfreq[i].f > 0) {
            node = create_node();
            if (node) {
                node->freq = hfreq[i].f;
                node->ch = hfreq[i].b;
                insert(node);
                hufflist[node->ch] = node;
            }
            else {
                fprintf(stderr, "\nMemory allocation error!!!\n");
                exit(0);
            }
            hcount++;    /* record the count of symbols. */
        }
    }
}

/*
This is a common function for Algorithm FGK
which is actually a pseudo-FGK.

The code here simply looks on the "uncle" node. True
Algorithm FGK looks on the "uncles" and *all* "cousins."

(I include it here for reference.)
*/
huffnode_t *get_highest_equal_node(huffnode_t *node) {
    huffnode_t *highest = NULL;

    if (node->parent->child_1 == node &&
            node->freq == node->parent->child_2->freq) {
        highest = node->parent->child_2;
    }

    /* Verify parent's sibling or "uncle." */
    if (
            node->parent->parent &&
                    node->parent->parent->child_1 == node->parent &&
                    node->parent->parent->child_2    /* uncle */
                            && node->freq == node->parent->parent->child_2->freq
            ) {
        highest = node->parent->parent->child_2;
    }
    else if (
            node->parent->parent &&
                    node->parent->parent->child_2 == node->parent &&
                    node->parent->parent->child_1    /* uncle */
                            && node->freq == node->parent->parent->child_1->freq
            ) {
        highest = node->parent->parent->child_1;
    }

    return highest;
}

/*
This function is used when the symbol node is the
sibling of the 0-node.  Separating this function
speeds up the get_highest_equal_nodeFGK() function,
which can then have only one conditional test.
*/
huffnode_t *get_highest_equal_LEAF_FGK(huffnode_t *node) {
    huffnode_t *highest = NULL;
    int i;

    /* start from the next numbered node. */
    for (i = node->number + 1; i < ROOT_NODE_NUMBER; i++) {
        if (    /* Equal count? */
                node->freq == numbers[i]->freq) {
            if (
                /* "leaf" nodes only! */
                    numbers[i]->ch != INTERNAL_NODE
                    )
                highest = numbers[i];
        }
        else break;
    }
    return highest;
}

/*
This is the heart of Algorithm FGK; it finds the
highest node of equal count with the current node.

This is a TRUE FGK implementation of getting the
highest node.
*/
huffnode_t *get_highest_equal_nodeFGK(huffnode_t *node) {
    huffnode_t *highest = NULL;
    int i;

    /* start from the next numbered node. */
    for (i = node->number + 1; i < ROOT_NODE_NUMBER; i++) {
        /* Equal count? */
        if (node->freq == numbers[i]->freq)
            highest = numbers[i];
        else break;
    }

    return highest;
}

/*
This code for Update is meant to be readable and follows the
pseudo-code for Algorithm FGK as described by J.S. Vitter in
his paper on Algorithm V.
*/
void update_treeFGK(int c) {
    huffnode_t *highest = NULL, *node = hufflist[c];

    if (node == NULL) {
        create_new_zero_node(c);
        node = hufflist[c];    /* right child just created. */
    }

    /* if a sibling of the 0-node. */
    if (node->parent == zero_node->parent) {
        highest = get_highest_equal_LEAF_FGK(node);
        if (highest) {
            swap_nodes(node, highest);
        }

        /* increment the node's count. */
        node->freq++;

        /* traverse up the tree. */
        node = node->parent;
    }

    while (node != top) {
        highest = get_highest_equal_nodeFGK(node);
        if (highest) {
            swap_nodes(node, highest);
        }

        /* increment the node's count. */
        node->freq++;

        /* traverse up the tree. */
        node = node->parent;
    }
}

listnode_t *get_least_node(void) {
    listnode_t *node;

    node = list;            /* get the least-freq node. */
    if (list) {
        list = list->next;  /* move to next node.       */
    }
    return node;
}

void create_hufftree(void) {
    listnode_t *node1, *node2, *parent;

    /* make sure all the node addresses are NULL. */
    init_hufflist();

    create_symbol_list();  /* create the initial list of symbols. */

    /*
    then create the Huffman tree,
    as long as there are nodes in the list.
    */
    while (list) {
        /* get the first least-freq node. */
        node1 = get_least_node();
        if (list) {
            /*
            a node is still available in the list.
            assign the second least-freq node.
            */
            node2 = get_least_node();

            /* create the parent node. */
            parent = create_node();

            /* create the links. */
            node1->parent = parent;
            node2->parent = parent;
            parent->child_1 = node1;
            parent->child_2 = node2;

            /* add frequency counts. */
            parent->freq = node1->freq + node2->freq;

            /* two nodes ignored, insert the new parent node. */
            insert(parent);
        }
        else {
            top = node1;
            if (node1 == list_head) {
                top = create_node();
                top->child_1 = node1;
                top->child_2 = node1;
                node1->parent = top;
            }
            break;
        }
    }
}

/* this is the one used by the compression program. */
void hcompress(uint32_t *bitword, uint8_t *shift, listnode_t *node) {
    //int bit_cnt = 0;
    //char bit[H_MAX];    /* a stack. */

    if (!node) return;

    while (node->parent) {
        if (node == node->parent->child_1) {
            //bit[bit_cnt] = 0;    /* push. */
        } else if (node == node->parent->child_2) {
            *bitword |= (1 << *shift);//bit[bit_cnt] = 1;    /* push. */
        }
        *shift = *shift + 1;
        //bit_cnt++;    /* increment count. */
        node = node->parent;
    }

    /* now, output the bits in reverse for easy decompression. */
//    while (bit_cnt--) {          /* decrement count and pop. */
//        if (bit[bit_cnt]) {
//            *bitword |= (1 << *shift);
//            //put_ONE();            /* output a ONE (1) bit. */
//        }
//        else {
//            //put_ZERO();           /* output a ZERO (0) bit. */
//        }
//        *shift ++;
//    }
}

uint_fast64_t count = 0;
uint_fast64_t distance = 0;

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
    //uint32_t chunk1;

    if (*predictedChunk ^ chunk) {
        density_argonaut_dictionary_chunk_entry *found = &state->dictionary.chunks[*hash];
        uint32_t *found_a = &found->chunk_a;
        if (*found_a ^ chunk) {
            //uint32_t hashalt = *hash;//((*hash >> (16 - DENSITY_SECONDARY_HASH_BITS)) ^ *hash) & ((1 << DENSITY_SECONDARY_HASH_BITS) - 1);//&found->hash8;// (*hash >> 8) ^ *hash;
            //uint32_t *found_b = &(state->dictionary.altchunks[hashalt].chunk);
            uint32_t *found_b = &found->chunk_b;
            //uint32_t* found_b = &(found->alt->chunk);
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

                /****uint8_t letterA = (uint8_t) (chunk & 0xFF);
                uint8_t letterB = (uint8_t) ((chunk & 0xFF00) >> 8);
                uint8_t letterC = (uint8_t) ((chunk & 0xFF0000) >> 16);
                uint8_t letterD = (uint8_t) ((chunk & 0xFF000000) >> 24);*****/

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


                //* (uint32_t*)(out->pointer) = DENSITY_LITTLE_ENDIAN_32(chunk);
                //out->pointer += sizeof(uint32_t);

                const uint32_t chunkRS8 = chunk >> 8;
                const uint32_t chunkRS16 = chunk >> 16;
                const uint32_t chunkRS24 = chunk >> 24;

                const uint16_t bigramP = (uint16_t) ((state->lastChunk >> 24) | ((chunk & 0xFF) << 8));
                const uint16_t bigramA = (uint16_t) (chunk & 0xFFFF);
                const uint16_t bigramB = (uint16_t) (chunkRS8 & 0xFFFF);
                const uint16_t bigramC = (uint16_t) (chunkRS16 & 0xFFFF);

                const uint8_t hashP = (uint8_t) (((bigramP * DENSITY_ARGONAUT_HASH32_MULTIPLIER) >> (32 - DCT_BITS)));
                const uint8_t hashA = (uint8_t) (((bigramA * DENSITY_ARGONAUT_HASH32_MULTIPLIER) >> (32 - DCT_BITS)));
                const uint8_t hashB = (uint8_t) (((bigramB * DENSITY_ARGONAUT_HASH32_MULTIPLIER) >> (32 - DCT_BITS)));
                const uint8_t hashC = (uint8_t) (((bigramC * DENSITY_ARGONAUT_HASH32_MULTIPLIER) >> (32 - DCT_BITS)));

                density_argonaut_encode_push_to_signature(out, state, 0x1, 2);
                if (state->dictionary.dct[hashA] == bigramA) {
                    *(out->pointer) = hashA;
                    out->pointer++;
                    //density_argonaut_encode_push_to_signature(out, state, hashA, DCT_BITS);
                } else {
                    //int ch; int symbol;
                    //ch = (uint8_t) (chunk & 0xFF);			/* Read the next character. */
                    //symbol = char_to_index[ch];		/* Translate to an index.   */
                    //encode_symbol(out, state, symbol,cum_freq);		/* Encode that symbol.      */
                    //update_model(symbol);
                    //ch = (uint8_t) ((chunk & 0xFF00) >> 8);			/* Read the next character. */
                    //symbol = char_to_index[ch];		/* Translate to an index.   */
                    //encode_symbol(out, state, symbol,cum_freq);		/* Encode that symbol.      */
                    //update_model(symbol);

                    const uint8_t letterA = (uint8_t) (chunk & 0xFF);
                    const uint8_t letterB = (uint8_t) (chunkRS8 & 0xFF);
                    //printf("%c%c", letterA, letterB);

                    /*const uint8_t hashLA = (uint8_t) (((letterA * DENSITY_ARGONAUT_HASH32_MULTIPLIER) >> (32 - DCTL_BITS)));
                    const uint8_t hashLB = (uint8_t) (((letterB * DENSITY_ARGONAUT_HASH32_MULTIPLIER) >> (32 - DCTL_BITS)));

                    density_argonaut_encode_push_to_signature(out, state, 0x1, 1);
                    if (state->dictionary.dctl[hashLA] == letterA) {
                        density_argonaut_encode_push_to_signature(out, state, hashLA, DCTL_BITS);
                    } else {
                        state->dictionary.dctl[hashLA] = letterA;
                        *(out->pointer) = letterA;
                        out->pointer++;
                    }
                    if (state->dictionary.dctl[hashLB] == letterB) {
                        density_argonaut_encode_push_to_signature(out, state, hashLB, DCTL_BITS);
                    } else {
                        state->dictionary.dctl[hashLB] = letterB;
                        *(out->pointer) = letterB;
                        out->pointer++;
                    }*/

                    /****const density_argonaut_dictionary_letter_entry *lA = &state->dictionary.letters[letterA];
                    density_argonaut_dictionary_letter_entry *lB = &state->dictionary.letters[letterB];

                    const density_argonaut_huffman_code codeA = density_argonaut_huffman_codes[lA->rank];
                    const density_argonaut_huffman_code codeB = density_argonaut_huffman_codes[lB->rank];

                    density_argonaut_encode_push_to_signature(out, state, codeA.code | (codeB.code << codeA.bitSize), codeA.bitSize + codeB.bitSize);

                    //density_argonaut_encode_update_letter_rank(lA, state);
                    density_argonaut_encode_update_letter_rank(lB, state);****/

                    //i++;

                    /*uint32_t word = 0;
                    uint8_t shift = 0;
                    if (density_unlikely(first)) {
                        update_treeFGK(letterA);
                        first = false;
                    } else {
                        if (density_likely(hufflist[letterA])) {
                            hcompress(&word, &shift, hufflist[letterA]);
                            density_argonaut_encode_push_to_signature(out, state, word, shift);
                        } else {
                            hcompress(&word, &shift, zero_node);
                            density_argonaut_encode_push_to_signature(out, state, word, shift);
                            *(out->pointer) = letterA;
                        }
                        update_treeFGK(letterA);
                    }

                    word = 0;
                    shift = 0;
                    if (density_likely(hufflist[letterB])) {
                        hcompress(&word, &shift, hufflist[letterB]);
                        density_argonaut_encode_push_to_signature(out, state, word, shift);
                    } else {
                        hcompress(&word, &shift, zero_node);
                        density_argonaut_encode_push_to_signature(out, state, word, shift);
                        *(out->pointer) = letterB;
                    }
                    update_treeFGK(letterB);*/

                    //*(uint16_t *) (out->pointer) = bigramA;
                    //out->pointer += sizeof(uint16_t);

                    /*distance += (count - state->dictionary.indexes[letterA]) + (count + 1 - state->dictionary.indexes[letterB]);

                    state->dictionary.indexes[letterA] = count;
                    state->dictionary.indexes[letterB] = count + 1;

                    count += 2;*/
                }

                if (state->dictionary.dct[hashC] == bigramC) {
                    *(out->pointer) = hashC;
                    out->pointer++;
                    //density_argonaut_encode_push_to_signature(out, state, hashC, DCT_BITS);
                } else {
                    //int ch; int symbol;
                    //ch = (uint8_t) ((chunk & 0xFF0000) >> 16);			/* Read the next character. */
                    //symbol = char_to_index[ch];		/* Translate to an index.   */
                    //encode_symbol(out, state, symbol,cum_freq);		/* Encode that symbol.      */
                    //update_model(symbol);
                    //ch = (uint8_t) ((chunk & 0xFF000000) >> 24);			/* Read the next character. */
                    //symbol = char_to_index[ch];		/* Translate to an index.   */
                    //encode_symbol(out, state, symbol,cum_freq);		/* Encode that symbol.      */
                    //update_model(symbol);

                    const uint8_t letterC = (uint8_t) (chunkRS16 & 0xFF);
                    const uint8_t letterD = (uint8_t) (chunkRS24 & 0xFF);
                    //printf("%c%c", letterC, letterD);

                    /*const uint8_t hashLC = (uint8_t) (((letterC * DENSITY_ARGONAUT_HASH32_MULTIPLIER) >> (32 - DCTL_BITS)));
                    const uint8_t hashLD = (uint8_t) (((letterD * DENSITY_ARGONAUT_HASH32_MULTIPLIER) >> (32 - DCTL_BITS)));

                    density_argonaut_encode_push_to_signature(out, state, 0x1, 1);
                    if (state->dictionary.dctl[hashLC] == letterC) {
                        density_argonaut_encode_push_to_signature(out, state, hashLC, DCTL_BITS);
                    } else {
                        state->dictionary.dctl[hashLC] = letterC;
                        *(out->pointer) = letterC;
                        out->pointer++;
                    }
                    if (state->dictionary.dctl[hashLD] == letterD) {
                        density_argonaut_encode_push_to_signature(out, state, hashLD, DCTL_BITS);
                    } else {
                        state->dictionary.dctl[hashLC] = letterD;
                        *(out->pointer) = letterD;
                        out->pointer++;
                    }*/

                    /****density_argonaut_dictionary_letter_entry *lC = &state->dictionary.letters[letterC];
                    const density_argonaut_dictionary_letter_entry *lD = &state->dictionary.letters[letterD];

                    density_argonaut_huffman_code codeC = density_argonaut_huffman_codes[lC->rank];
                    density_argonaut_huffman_code codeD = density_argonaut_huffman_codes[lD->rank];

                    density_argonaut_encode_push_to_signature(out, state, codeC.code | (codeD.code << codeC.bitSize), codeC.bitSize + codeD.bitSize);

                    density_argonaut_encode_update_letter_rank(lC, state);
                    //density_argonaut_encode_update_letter_rank(lD, state);****/

                    //i++;

                    /*uint32_t word = 0;
                    uint8_t shift = 0;
                    if (density_unlikely(first)) {
                        update_treeFGK(letterC);
                        first = false;
                    } else {
                        if (density_likely(hufflist[letterC])) {
                            hcompress(&word, &shift, hufflist[letterC]);
                            density_argonaut_encode_push_to_signature(out, state, word, shift);
                        } else {
                            hcompress(&word, &shift, zero_node);
                            density_argonaut_encode_push_to_signature(out, state, word, shift);
                            *(out->pointer) = letterC;
                        }
                        update_treeFGK(letterC);
                    }

                    word = 0;
                    shift = 0;
                    if (density_likely(hufflist[letterD])) {
                        hcompress(&word, &shift, hufflist[letterD]);
                        density_argonaut_encode_push_to_signature(out, state, word, shift);
                    } else {
                        hcompress(&word, &shift, zero_node);
                        density_argonaut_encode_push_to_signature(out, state, word, shift);
                        *(out->pointer) = letterD;
                    }
                    update_treeFGK(letterD);*/

                    //*(uint16_t *) (out->pointer) = bigramC;
                    //out->pointer += sizeof(uint16_t);

                    /*distance += (count - state->dictionary.indexes[letterC]) + (count + 1 - state->dictionary.indexes[letterD]);

                    state->dictionary.indexes[letterC] = count;
                    state->dictionary.indexes[letterD] = count + 1;

                    count += 2;*/
                }

                state->dictionary.dct[hashP] = bigramP;
                state->dictionary.dct[hashA] = bigramA;
                state->dictionary.dct[hashB] = bigramB;
                state->dictionary.dct[hashC] = bigramC;

                /*for(int i = 0; i < 4; i ++) {
                    printf("%c", out->pointer[i]);
                }
                printf(",");*/

                //uint32_t hashw;
                //uint32_t j;

                // 4 letters
                /*hashw = density_argonaut_hash(chunk, 0, 4);
                if(state->dictionary.dict[hashw].word == chunk)
                    density_argonaut_encode_push_to_signature(out, state, 0, 12);*/

                // 3 letters
                /*uint32_t chunkm;
                chunkm = density_argonaut_encode_mask_word(chunk, 3);
                hashw = density_argonaut_hash(chunkm, 3);
                //printf("%lu,", hashw);
                if (state->dictionary.dict[hashw].word == chunkm) {
                    density_argonaut_encode_push_to_signature(out, state, 0, 2 * DICT_BITS);
                    goto labelout;
                } else {
                    state->dictionary.dict[hashw].word = chunkm;
                }

                chunkm = density_argonaut_encode_mask_word(chunk >> 8, 3);
                hashw = density_argonaut_hash(chunkm, 3);
                if (state->dictionary.dict[hashw].word == chunkm) {
                    density_argonaut_encode_push_to_signature(out, state, 0, 2 * DICT_BITS);
                    goto labelout;
                } else {
                    state->dictionary.dict[hashw].word = chunkm;
                }

                // 2 letters
                chunkm = density_argonaut_encode_mask_word(chunk, 2);
                hashw = density_argonaut_hash(chunkm, 2);
                if (state->dictionary.dict[hashw].word == chunkm) {
                    density_argonaut_encode_push_to_signature(out, state, 0, DICT_BITS);
                    goto labelout2;
                } else {
                    state->dictionary.dict[hashw].word = chunkm;
                }

                chunkm = density_argonaut_encode_mask_word(chunk >> 8, 2);
                hashw = density_argonaut_hash(chunkm, 2);
                if (state->dictionary.dict[hashw].word == chunkm) {
                    density_argonaut_encode_push_to_signature(out, state, 0, 3 * DICT_BITS);
                    goto labelout;
                } else {
                    state->dictionary.dict[hashw].word = chunkm;
                }

                labelout2:
                chunkm = density_argonaut_encode_mask_word(chunk >> 16, 2);
                hashw = density_argonaut_hash(chunkm, 2);
                if (state->dictionary.dict[hashw].word == chunkm) {
                    density_argonaut_encode_push_to_signature(out, state, 0, DICT_BITS);
                    goto labelout;
                } else {
                    state->dictionary.dict[hashw].word = chunkm;
                }

                density_argonaut_encode_push_to_signature(out, state, 0, 4 * DICT_BITS);

                labelout:
                j++;*/
                //

                /******density_argonaut_huffman_code codeA = density_argonaut_huffman_codes[state->dictionary.letters[letterA].rank];
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
                density_argonaut_encode_update_letter_rank(&state->dictionary.letters[letterD], state);*****/

            } else {
                //found->usage++;
                //found->usage_b++;
                //found->durability_b++;
                density_argonaut_huffman_code code = density_argonaut_encode_fetch_form_rank_for_use(state, DENSITY_ARGONAUT_FORM_DICTIONARY_B);
                density_argonaut_encode_push_to_signature(out, state, code.code, code.bitSize/* + DENSITY_SECONDARY_HASH_BITS*/);
                //density_argonaut_encode_push_to_signature(out, state, 0x0, (uint8_t) (DENSITY_ARGONAUT_HASH_BITS - 8));

                *(uint16_t *) (out->pointer) = DENSITY_LITTLE_ENDIAN_16(*hash);
                out->pointer += sizeof(uint16_t);

                //*(out->pointer) = found->alt->hash;//(uint8_t) ((*hash >> 8) ^ *hash);
                //out->pointer += sizeof(uint8_t);
                //i++;

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
            //found->usage++;
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
    //state->formStatistics[DENSITY_ARGONAUT_FORM_PREDICTIONS].usage = 0;
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
        //state->dictionary.altchunks[count].hash = count;
    } while (count--);

    /*uint_fast16_t dcount = (1 << DENSITY_SECONDARY_HASH_BITS);
    do {
        uint16_t hash = (uint16_t)((dcount >> (16 - DENSITY_SECONDARY_HASH_BITS)) ^ dcount);
        state->dictionary.chunks[dcount].alt = &(state->dictionary.altchunks[hash]);
    } while (dcount--);*/

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

    /*uint16_t diCount = 0xffff;
    do {
        uint16_t hash = (uint16_t)((diCount >> (16 - DENSITY_SECONDARY_HASH_BITS)) ^ diCount);
        state->dictionary.chunks[diCount].alt = &(state->dictionary.altchunks[hash]);
    } while (diCount--);*/

    state->lastHash = 0;
    state->lastChunk = 0;
    state->beforeLastHash = 0;

    //start_model();
    //start_encoding();

    init_hufflist();
    top = zero_node = create_node();

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

    //done_encoding(out, state);

    return DENSITY_KERNEL_ENCODE_STATE_READY;
}