/**
 * Decodes a single CELT frame and dumps human-readable information about it to stdout.
 */

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "range_coder.h"
#include "symbol_context.h"
#include "celt_util.h"

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

    opus_raw_frame_t* frame;
    if ((frame = opus_raw_frame_create_from_nth_in_file(ar->filename, ar->frame_number)) == NULL) {
        printf("error opening opus file %s\n", ar->filename);
        printf("usage: %s <filename> <frame-number>\n", argv[0]);
        return -1;
    }

    range_decoder_t* range_decoder = range_decoder_create(frame->data + 1, frame->length - 1);

    // Initialize frame context info.
    // TODO: should just have a single function that consumes most of the symbols right at the start
    // and generates a frame_context_t.
    // This will by necessity exclude spread_decision, band boosts, trim, anti_collapse, and skip,
    // which would probably be more appropriate in a different structure anyways.
    frame_context_t context;
    uint8_t toc = frame->data[0];
    context.LM = (toc >> 3) & 0x03;
    context.C = 1;

    frame_context_initialize_band_boundaries(&context);
    frame_context_initialize_caps(&context);

    // Decode 'static' symbols for frame context
    context.silence = range_decoder_decode_symbol(range_decoder, &CELT_silence_context); printf("\n");
    context.post_filter = range_decoder_decode_symbol(range_decoder, &CELT_post_filter_context); printf("\n");
    if (context.post_filter) {
        printf("post-filter specified in frame but not implemented!\n");
        return -1;
    }
    context.transient = range_decoder_decode_symbol(range_decoder, &CELT_transient_context); printf("\n");
    context.intra = range_decoder_decode_symbol(range_decoder, &CELT_intra_context); printf("\n");

    // Decode coarse energy
    int32_t coarse[21];
    float energy[21];
    coarse_energy_symbols_decode(&context, range_decoder, coarse);
    coarse_energy_calculate_from_symbols(&context, coarse, energy, NULL);

    // tf_change, tf_select!
    int32_t tf_changes[21];
    decode_tf_changes(&context, range_decoder, tf_changes); printf("\n");
    printf("tf_changes: {");
    for (int i = 0; i < 21; printf("% 3i,", tf_changes[i++]));
    printf("}\n");

    context.spread_decision = range_decoder_decode_symbol(range_decoder, &CELT_spread_context); printf("\n");

    // Calculate band boosts
    int32_t* boosts = calloc(21, sizeof(int32_t));
    decode_band_boosts(&context, range_decoder, boosts);
    int32_t trim = range_decoder_decode_symbol(range_decoder, &CELT_trim_context); printf("\n");

    printf("boosts: {");
    for (int i = 0; i < 21; printf("% 3i,", boosts[i++]));
    printf("}\n");
    printf("allocation trim: % 2i\n", trim);

    // Figure out bit allocation for fine energy and PVQ


    range_decoder_destroy(range_decoder);

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
 * Returns
 */
static int get_band_boost_quanta(int width) {
    int quanta = 8 * width;
    if (quanta >= (6 << 3)) quanta = (6 << 3);
    if (quanta <= width) quanta = width;

    return quanta;
}

/**
 * Given a symbol_context_t which decodes
 *     '0' with probability 2^n - 1 / 2^n
 *     '1' with probability       1 / 2^n
 *
 * This function will modify the symbol so that the 1 becomes twice as likely, i.e. 'n' in the
 * above CDF will be divided by 2.
 *
 * e.g. {31, 1} / 32 becomes {15, 1} / 16
 */
static void double_1_probability(symbol_context_t* sym) {
    sym->ft /= 2;

    sym->fl[0] /= 2;
    sym->fl[1] /= 2;
    sym->fh[0] /= 2;
    sym->fh[1] /= 2;
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

/**
 * Given a range decoder whose read head is pointing at band boosts as described in RFC6716 section
 * 4.3.3, this function decodes the band boosts and advances the range decoder.
 *
 * @param[in]     context    frame_context holding some basic info about the frame; all of its
 *                           fields must be filled out properly for this function to operate.
 * @param[in,out] rd         range_decoder whose bitstream shall be read to decode band boosts.
 * @param[out]    boosts     Should point to an array of 21 int32_t. After this function executes,
 *                           the ith element of 'boosts' will hold the amount of band boost in
 *                           1/8th bits for band i.
 */
void decode_band_boosts(const frame_context_t* context, range_decoder_t* rd, int32_t* boosts)
{
    symbol_context_t* initial_boost_context = symbol_context_create_minprob_1(6, "initial boost");
    symbol_context_t* subsequent_boost_context = symbol_context_create_minprob_1(1, "subsequent boost");

    for (int i = 0; i < 21; i++) {
        int width = context->C * (context->band_boundaries[i + 1] - context->band_boundaries[i]);
        int quanta = get_band_boost_quanta(width);
        boosts[i] = 0;

        // calculate boost for this band
        bool did_boost = false;
        const symbol_context_t* boost_context = initial_boost_context;
        do {
            uint32_t do_boost = range_decoder_decode_symbol(rd, boost_context);
            if (do_boost) {
                did_boost = true;
                boosts[i] += quanta;
                boost_context = subsequent_boost_context;
            } else {
                break;
            }
        } while (1);

        if (did_boost && (initial_boost_context->ft > 4)) {
            // if we boosted a band, boosts are twice as likely for subsequent bands (unless the
            // boost has probability 1/4, in which case the likelihood saturates).
            double_1_probability(initial_boost_context);
        }
    }

    symbol_context_destroy(initial_boost_context);
    symbol_context_destroy(subsequent_boost_context);
}
