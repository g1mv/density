/*
 * Centaurean Density
 *
 * Copyright (c) 2015, Guillaume Voirin
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     1. Redistributions of source code must retain the above copyright notice, this
 *        list of conditions and the following disclaimer.
 *
 *     2. Redistributions in binary form must reproduce the above copyright notice,
 *        this list of conditions and the following disclaimer in the documentation
 *        and/or other materials provided with the distribution.
 *
 *     3. Neither the name of the copyright holder nor the names of its
 *        contributors may be used to endorse or promote products derived from
 *        this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * 9/03/15 11:19
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

#include "kernel_lion_form_model.h"

const density_lion_entropy_code density_lion_form_entropy_codes[DENSITY_LION_NUMBER_OF_FORMS] = DENSITY_LION_FORM_MODEL_ENTROPY_CODES;

DENSITY_WINDOWS_EXPORT DENSITY_FORCE_INLINE void density_lion_form_model_init(density_lion_form_data *data) {
    density_lion_form_node* rank_0 = &data->formsPool[0];
    rank_0->form = DENSITY_LION_FORM_SECONDARY_ACCESS;
    rank_0->usage = 0;
    rank_0->rank = 0;
    rank_0->previousForm = NULL;
    data->formsIndex[DENSITY_LION_FORM_SECONDARY_ACCESS] = rank_0;

    density_lion_form_node* rank_1 = &data->formsPool[1];
    rank_1->form = DENSITY_LION_FORM_CHUNK_DICTIONARY_A;
    rank_1->usage = 0;
    rank_1->rank = 1;
    rank_1->previousForm = rank_0;
    data->formsIndex[DENSITY_LION_FORM_CHUNK_DICTIONARY_A] = rank_1;

    density_lion_form_node* rank_2 = &data->formsPool[2];
    rank_2->form = DENSITY_LION_FORM_CHUNK_DICTIONARY_B;
    rank_2->usage = 0;
    rank_2->rank = 2;
    rank_2->previousForm = rank_1;
    data->formsIndex[DENSITY_LION_FORM_CHUNK_DICTIONARY_B] = rank_2;

    density_lion_form_node* rank_3 = &data->formsPool[3];
    rank_3->form = DENSITY_LION_FORM_CHUNK_PREDICTIONS;
    rank_3->usage = 0;
    rank_3->rank = 3;
    rank_3->previousForm = rank_2;
    data->formsIndex[DENSITY_LION_FORM_CHUNK_PREDICTIONS] = rank_3;

    density_lion_form_node* rank_4 = &data->formsPool[4];
    rank_4->form = DENSITY_LION_FORM_CHUNK_SECONDARY_PREDICTIONS;
    rank_4->usage = 0;
    rank_4->rank = 4;
    rank_4->previousForm = rank_3;
    data->formsIndex[DENSITY_LION_FORM_CHUNK_SECONDARY_PREDICTIONS] = rank_4;
}

DENSITY_WINDOWS_EXPORT DENSITY_FORCE_INLINE void density_lion_form_model_update(density_lion_form_data *restrict data, density_lion_form_node *restrict form, density_lion_form_node *restrict previous_form) {
    if (density_unlikely(previous_form->usage < form->usage)) {    // Relative stability is assumed
        const DENSITY_LION_FORM form_value = form->form;

        const DENSITY_LION_FORM previous_form_value = previous_form->form;
        const uint32_t previous_form_usage = previous_form->usage;

        previous_form->form = form_value;
        previous_form->usage = form->usage;

        form->form = previous_form_value;
        form->usage = previous_form_usage;

        data->formsIndex[form_value] = previous_form;
        data->formsIndex[previous_form_value] = form;
    }
}

DENSITY_WINDOWS_EXPORT DENSITY_FORCE_INLINE density_lion_entropy_code density_lion_form_model_get_encoding(density_lion_form_data *data, const DENSITY_LION_FORM form) {
    density_lion_form_node *form_found = data->formsIndex[form];
    form_found->usage++;

    density_lion_form_node *previous_form = form_found->previousForm;
    if (form_found->previousForm) {
        const uint8_t rank = form_found->rank;

        density_lion_form_model_update(data, form_found, previous_form);

        return density_lion_form_entropy_codes[rank];
    } else
        return density_lion_form_entropy_codes[0];
}