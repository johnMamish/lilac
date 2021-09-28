/**
 * Decodes a single CELT frame and dumps human-readable information about it to stdout.
 */

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "anti_collapse.h"
#include "bit_allocation.h"
#include "range_coder.h"
#include "symbol_context.h"
#include "celt_util.h"
#include "fine_energy.h"

#define ARRAY_NUMEL(arr) (sizeof(arr) / (sizeof(arr[0])))

typedef struct arg_results {
    char* filename;
    int   frame_number;
} arg_results_t;

arg_results_t* arg_results_create_from_argc_argv(int argc, char** argv);
void arg_results_destroy(arg_results_t* out);

void coarse_energy_symbols_decode(const frame_context_t* context, range_decoder_t* rd, int32_t* out);
void coarse_energy_calculate_from_symbols(const frame_context_t* context, const int32_t* symbols,
                                          float* out, const float* prev_energy);
void decode_tf_changes(const frame_context_t* context, range_decoder_t* rd, int32_t* tf_changes);
void decode_band_boosts(const frame_context_t* context, range_decoder_t* rd, int32_t* boosts);

int main(int argc, char** argv)
{
    // parse arguments
    arg_results_t* ar;
    if ((ar = arg_results_create_from_argc_argv(argc, argv)) == NULL) {
        return -1;
    }


    printf("decoding frame %i\n"
           "================================================\n", ar->frame_number);
    opus_raw_frame_t* frame;
    if ((frame = opus_raw_frame_create_from_nth_in_file(ar->filename, ar->frame_number)) == NULL) {
        printf("error opening opus file %s\n", ar->filename);
        printf("usage: %s <filename> <frame-number>\n", argv[0]);
        return -1;
    }

    uint8_t toc = frame->data[0];
    range_decoder_t* range_decoder = range_decoder_create(frame->data + 1, frame->length - 1);

    // Initialize frame context info.
    frame_context_t* context;
    if ((context = frame_context_create_and_read_basic_info(toc, range_decoder)) == NULL) {
        return -1;
    }

    // Decode coarse energy
    int32_t coarse[21];
    float energy[21];
    coarse_energy_symbols_decode(context, range_decoder, coarse);
    coarse_energy_calculate_from_symbols(context, coarse, energy, NULL);

    // tf_change, tf_select
    int32_t tf_changes[21];
    decode_tf_changes(context, range_decoder, tf_changes); printf("\n");
    printf("tf_changes: {");
    for (int i = 0; i < 21; printf("% 3i,", tf_changes[i++]));
    printf("}\n");

    int spread_decision = range_decoder_decode_symbol(range_decoder, &CELT_spread_context); printf("\n");
    printf("spread: %i\n", spread_decision);

    // Figure out bit allocation for fine energy and PVQ
    bit_allocation_description_t* bits;
    reserve_anti_collapse_bit_from_range_decoder(context, range_decoder);
    bits = bit_allocation_create(context, range_decoder);

    printf("final bit allocations: ");
    printf("pvq: {");
    for (int j = 0; j < 21; j++) { printf("% 5d", bits->bands[j].pvq_bits); }
    printf("}, ");
    printf("fine: {");
    for (int j = 0; j < 21; j++) { printf("% 5d", bits->bands[j].energy_bits); }
    printf("}, ");
    printf("balance: % 5d\n", bits->balance);

    fine_energy_t* fine_energy = fine_energy_create_from_range_decoder(context, bits, range_decoder);
    printf("fine energy: {");
    for (int i = 0; i < 21; printf("%11.6f", fine_energy->energies[i++]));
    printf("}\n");

    range_decoder_destroy(range_decoder);

    printf("\n\n");

    return 0;
}

/**
 * Implements command line argument parsing.
 *
 * On success, returns a newly created arg_results_t.
 * On failure, prints error messages and returns NULL.
 */
arg_results_t* arg_results_create_from_argc_argv(int argc, char** argv)
{
    if (argc != 3) {
        printf("usage: %s <filename> <frame-number>\n", argv[0]);
        return NULL;
    }

    arg_results_t* result = calloc(1, sizeof(arg_results_t));

    result->filename = strndup(argv[1], (1 << 16));

    char* endptr;
    result->frame_number = strtol(argv[2], &endptr, 10);
    if (*endptr != '\0') {
        arg_results_destroy(result);
        printf("Second arg needs to be a base-10 number.\n");
        printf("usage: %s <filename> <frame-number>\n", argv[0]);
        return NULL;
    }

    return result;
}

void arg_results_destroy(arg_results_t* out)
{
    if (out != NULL) free(out->filename);
    free(out);
}

/**
 * Decodes coarse energy symbols for a given frame. These symbols just represent the residual from
 * an implicit prediction filter (see rfc6716 section 4.3.2.1). In intra-frame mode, the
 * prediction filter should predict band i from bands [0, i), and the decoded symbol should be
 * added to the prediction to get the correct value.
 *
 * @param[in]     context    Struct containing basic frame info like frame size & number of channels.
 *                           This info determines which parameters will generate the laplace-like
 *                           distribution representing the CDF of the coarse energy residual symbols
 * @param[in,out] rd         Range decoder which is advanced to the point where it's about to read
 *                           coarse energy.
 * @param[out]    out        Caller should point this to an array containing 21 elements. Returns
 *                           the coarse energy symbols. These are just the symbols, NOT the coarse
 *                           energy values in db. They represent residuals which need to be passed
 *                           into the prediction filter.
 */
void coarse_energy_symbols_decode(const frame_context_t* context, range_decoder_t* rd, int32_t* out)
{
    for (int j = 0; j < 21; j++) {
        symbol_context_t* coarse_symbol_j = symbol_context_create_from_laplace(context, j);

        int32_t sym = range_decoder_decode_symbol(rd, coarse_symbol_j);
        if (sym & (1 << 0)) {
            out[j] = -((sym + 1) / 2);
        } else {
            out[j] = (sym / 2);
        }
        printf("decoded %i\n", out[j]);
        printf("\n");

        symbol_context_destroy(coarse_symbol_j);
    }
}

/**
 *
 */
void coarse_energy_calculate_from_symbols(const frame_context_t* context, const int32_t* symbols,
                                          float* out, const float* prev_energy)
{
    float beta = 4915.f / 32768.f;
    float prediction = 0.f;
    for (int i = 0; i < 21; i++) {
        out[i] = prediction + (float)(symbols[i]);
        prediction = out[i] - beta * ((float)symbols[i]);
        printf("[unquant_coarse_energy] band %i: decoded residual: %i, decoded energy: %f\n", i, symbols[i], out[i]);
    }
}

/**
 * Given a range decoder whose read head is pointing at the point in the bitstream where tf_change
 * and tf_select are encoded, this function decodes tf_change and tf_select and advances the range
 * decoder.
 *
 * @param[in]     context    frame_context holding some basic info about the frame; all of its
 *                           fields must be filled out properly for this function to operate.
 * @param[in,out] rd         range_decoder whose bitstream shall be read.
 * @param[out]    tf_changes Should point to an array of 21 int32_t. Returns tf_changes as described
 *                           in RFC6716 section 4.3.4.5.
 */
void decode_tf_changes(const frame_context_t* context, range_decoder_t* rd, int32_t* tf_changes)
{
    symbol_context_t* initial_tf_change_context;
    symbol_context_t* next_tf_change_context;
    if (context->transient) {
        initial_tf_change_context = symbol_context_create_minprob_1(2, "initial tf change context");
        next_tf_change_context = symbol_context_create_minprob_1(4, "subsequent tf change context");
    } else {
        initial_tf_change_context = symbol_context_create_minprob_1(4, "initial tf change context");
        next_tf_change_context = symbol_context_create_minprob_1(5, "subsequent tf change context");
    }

    symbol_context_t* tf_change_context = initial_tf_change_context;
    int curr = 0;
    bool tf_changed = false;
    uint32_t band_tf_change[21];
    for (int i = 0; i < 21; i++) {
        // The reference implementation included with the RFC does ^= even though the RFC itself
        // would seem to imply +=.
        const uint32_t tf_change = range_decoder_decode_symbol(rd, tf_change_context);
        curr ^= tf_change;
        tf_changed |= tf_change;

        band_tf_change[i] = curr;

        // After first band, we switch to a different CDF for decoding
        tf_change_context = next_tf_change_context;
    }

    // Section 4.3.1: The tf_select flag uses a 1/2 probability, but is only decoded if it can have
    // an impact on the result knowing the value of all per-band tf_change flags.
    int tf_select = 0;
    if (get_tf_change(context, false, tf_changed) != get_tf_change(context, true, tf_changed)) {
        // tf_select was worth decoding, so do it.
        // The reference code assumes that if this test fails with tf_changed == 1, that implies
        // that it will also fail with tf_changed == 0. This is true given the specific values in
        // tf_select_table, but if these values are changed in an alternative implementation,
        // this assumption will fail and this code can give inconsistent results in some edge cases.
        symbol_context_t* tf_select_symbol_context = symbol_context_create_minprob_1(1, "tf select");
        tf_select = range_decoder_decode_symbol(rd, tf_select_symbol_context);
        symbol_context_destroy(tf_select_symbol_context);
    }

    // finalize values
    for (int i = 0; i < 21; i++) {
        tf_changes[i] = get_tf_change(context, tf_select, band_tf_change[i]);
    }

    symbol_context_destroy(initial_tf_change_context);
    symbol_context_destroy(next_tf_change_context);
}
