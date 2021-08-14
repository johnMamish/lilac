#include <stdlib.h>
#include <stdio.h>

#include "bit_allocation.h"
#include "celt_defines.h"

/**
 * Bit allocation is determined by
 *   - remaining number of bits in frame
 *   - band boosts
 *   - allocation tilt
 *   - band skips
 *
 * This structure holds some of that information
 */
typedef struct bit_allocation_parameters {
    int32_t* boosts;
    int32_t* pvq_threshold;
    int32_t* trim_offsets;
} bit_allocation_parameters_t;

/**
 * Determines the number of relative bits to remove from each band according to the
 * "Allocation Trim". The significance of the "allocation trim" parameter is described in section
 * 4.3.3 of RFC6716: values below 5 allocate more bits for lower frequency bands, values above
 * 5 allocate more bits for higher frequency bands, and a value of 5 gives flat allocation.
 *
 * Also determines the minimum number of bits that should be used for encoding shape (PVQ). For some
 * bands, if a very small number of bits (1/8 per MDCT bin) are provided for shape encoding, it's
 * not worth it to encode shape.
 *
 * @param[in]     alloc_trim Trim parameter on [0, 10]; values below 5 bias allocation toward
 *                           lower frequency bands, values above 5 bias allocation toward higher
 *                           freq bands, 5 is flat allocation.
 * @param[out]    thresh     Returns the minimum number of bits that's worthwhile to use for shape
 *                           encoding.
 * @param[out]    trim_offset  Returns the allocation trim for each band in 1/8th (???) bits.
 */

static void calculate_trim_offset(const frame_context_t* fc, int32_t alloc_trim, int32_t* trim_offset)
{
    int j;
    for (j = 0; j < 21; j++) {
        // N - number of MDCT buckets in band j
        const int N = fc->band_boundaries[j + 1] - fc->band_boundaries[j];

        /* Tilt of the allocation curve */
        // determine the actual allocation value for band j.
        trim_offset[j] = (fc->C * N * (alloc_trim - 5 - fc->LM) * (21 - j - 1) * (1 << BITRES)) >> 6;

        /* Giving less resolution to single-coefficient bands because they get
           more benefit from having one coarse value per coefficient*/
        // note: the number of samples in the j-th band is given by (N << LM).
        // This is described in the very last paragraph of section 4.3.3 of RFC6716; it can be found
        // by searching the document for "trim_offset"
        if ((N << fc->LM) == 1)
            trim_offset[j] -= fc->C << BITRES;
    }
}

/**
 * TODO: might want to move this to frame_context_t; pvq_thresh shouldn't be held seperately.
 */
static void calculate_pvq_thresh(const frame_context_t* fc, int32_t* thresh)
{
    int j;
    for (j = 0; j < 21; j++) {
        // N - number of MDCT buckets in band j
        const int N = fc->band_boundaries[j + 1] - fc->band_boundaries[j];

        /* Below this threshold, we're sure not to allocate any PVQ bits */
        // "For each coded band, set thresh[band] to 24 times the number of MDCT bins in the band and
        //  divide by 16.  If 8 times the number of channels is greater, use that instead.  This sets
        //  the minimum allocation to one bit per channel or 48 128th bits per MDCT bin, whichever
        //  is greater."
        thresh[j] = ((3 * N) << BITRES) >> 4;
        if (thresh[j] < (fc->C << BITRES)) {
            thresh[j] = (fc->C << BITRES);
        }
    }
}

/**
 * "Rows indicate the MDCT bands, columns are the different quality (q)  parameters.  The units
 *  are 1/32 bit per MDCT bin."
 */
static const unsigned char band_allocation[][21] = {
     /*0  200 400 600 800  1k 1.2 1.4 1.6  2k 2.4 2.8 3.2  4k 4.8 5.6 6.8  8k 9.6 12k 15.6 */
    {   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,},
    {  90, 80, 75, 69, 63, 56, 49, 40, 34, 29, 20, 18, 10,  0,  0,  0,  0,  0,  0,  0,  0,},
    { 110,100, 90, 84, 78, 71, 65, 58, 51, 45, 39, 32, 26, 20, 12,  0,  0,  0,  0,  0,  0,},
    { 118,110,103, 93, 86, 80, 75, 70, 65, 59, 53, 47, 40, 31, 23, 15,  4,  0,  0,  0,  0,},
    { 126,119,112,104, 95, 89, 83, 78, 72, 66, 60, 54, 47, 39, 32, 25, 17, 12,  1,  0,  0,},
    { 134,127,120,114,103, 97, 91, 85, 78, 72, 66, 60, 54, 47, 41, 35, 29, 23, 16, 10,  1,},
    { 144,137,130,124,113,107,101, 95, 88, 82, 76, 70, 64, 57, 51, 45, 39, 33, 26, 15,  1,},
    { 152,145,138,132,123,117,111,105, 98, 92, 86, 80, 74, 67, 61, 55, 49, 43, 36, 20,  1,},
    { 162,155,148,142,133,127,121,115,108,102, 96, 90, 84, 77, 71, 65, 59, 53, 46, 30,  1,},
    { 172,165,158,152,143,137,131,125,118,112,106,100, 94, 87, 81, 75, 69, 63, 56, 45, 20,},
    { 200,200,200,200,200,200,200,200,198,193,188,183,178,173,168,163,158,153,148,129,104,}
};


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
 * Returns
 */
static int get_band_boost_quanta(int width) {
    int quanta = 8 * width;
    if (quanta >= (6 << 3)) quanta = (6 << 3);
    if (quanta <= width) quanta = width;

    return quanta;
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
static void decode_band_boosts(const frame_context_t* context, range_decoder_t* rd, int32_t* boosts)
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

/**
 * This function consumes information about band boosts and allocation trim from a range decoder
 * and uses that information to allocate and initialize a new bit_allocation_parameters_t.
 */
bit_allocation_parameters_t* bit_allocation_parameters_create(const frame_context_t* fc,
                                                              range_decoder_t* rd)
{
    bit_allocation_parameters_t* ret = calloc(1, sizeof(bit_allocation_parameters_t));

    // calculate boosts
    ret->boosts = calloc(21, sizeof(int32_t));
    decode_band_boosts(fc, rd, ret->boosts);
    printf("boosts: {");
    for (int i = 0; i < 21; printf("% 3i,", ret->boosts[i++]));
    printf("}\n");

    // calculate trim
    int32_t trim = range_decoder_decode_symbol(rd, &CELT_trim_context); printf("\n");
    printf("allocation trim: % 2i\n", trim);
    ret->trim_offsets = calloc(21, sizeof(int32_t));
    calculate_trim_offset(fc, trim, ret->trim_offsets);

    // calculate pvq thresholds
    ret->pvq_threshold = calloc(21, sizeof(int32_t));
    calculate_pvq_thresh(fc, ret->pvq_threshold);

    return ret;
}

void bit_allocation_parameters_destroy(bit_allocation_parameters_t* bap)
{
    free(bap->pvq_threshold);
    free(bap->trim_offsets);
    free(bap->boosts);
    free(bap);
}

/**
 * Given a certain
 *
 * Returns the total number of bits allocated to all bands.
 */
static int calculate_bits_for_quality(const frame_context_t* fc,
                                      const bit_allocation_parameters_t* bap,
                                      int q,
                                      int32_t* bits_out)
{
    int psum = 0;
    int done = 0;

    for (int j = 20; j >= 0; j--) {
        int bitsj;

        int N = fc->band_boundaries[j + 1] - fc->band_boundaries[j];

        // bitsj contains the number of 1/8th bits that would be allocated to the jth band for a
        // quality factor of 'q'.
        // In very high bit-rate situations, 'q' might exceed the highest allowed quality value.
        // In that case, we just set bitsj equal to the capacity.
        if (q >= 10) {
            bitsj = fc->cap[j];
        } else {
            // divide by 4 is because table 5 is in 1/32nd bits, not 1/8th bits.
            bitsj = (fc->C * N * band_allocation[q][j]) / 4;
        }

        // Incorporate the trim_offset
        if (bitsj > 0) {
            bitsj = bitsj + bap->trim_offsets[j];
            bitsj = (bitsj < 0) ? 0 : bitsj;
        }

        // Incorporate band boost.
        if (q > 0)
            bitsj += bap->boosts[j];

        if ((bitsj >= bap->pvq_threshold[j]) || done) {
            done = 1;

            /* Don't allocate more than we can actually use */
            if (bitsj > fc->cap[j])
                bitsj = fc->cap[j];
        } else {
            // If this band and all of the bands higher than it didn't exceed their PVQ threshold,
            //
            if (bitsj >= (fc->C << BITRES))
                bitsj = (fc->C << BITRES);
            else
                bitsj = 0;
        }

        bits_out[j] = bitsj;
        psum += bits_out[j];
    }

    return psum;
}

/**
 * The reference implementation uses a slightly different algorithm for determining per-band bit
 * allocations at a given quality in one part of the code.
 */
static int calculate_bits_for_quality2(const frame_context_t* fc,
                                       const bit_allocation_parameters_t* bap,
                                       int q,
                                       int32_t* bits_out)
{
    int psum = 0;
    int done = 0;

    for (int j = 0; j < 21; j++) {
        int bitsj;
        if (q >= 11) {
            bitsj = fc->cap[j];
        } else {
            int N = fc->band_boundaries[j + 1] - fc->band_boundaries[j];
            bitsj = (fc->C * N * band_allocation[q][j]) / 4;
        }

        // Incorporate the trim_offset
        if (bitsj > 0) {
            bitsj = bitsj + bap->trim_offsets[j];
            bitsj = (bitsj < 0) ? 0 : bitsj;
        }

        // Incorporate band boost.
        if (q > 0)
            bitsj += bap->boosts[j];

        // if (bap->boosts[j] > 0)
        //     skip_start = j;

        bits_out[j] = bitsj;
        psum += bits_out[j];
    }

    return psum;
}

/**
 * Do a binary search over the CELT Static allocation table to determine the largest integer value
 * of 'q' (the quality factor) which doesn't exceed the remaining frame size.
 *
 */
static int search_q_lo(const frame_context_t* fc,
                       const bit_allocation_parameters_t* bap,
                       int32_t total)
{
    int lo = 1;
    int hi = 11 - 1;
    do {
        int mid = (lo + hi) / 2;

        int32_t dummy[21] __attribute__((unused));
        int psum = calculate_bits_for_quality(fc, bap, mid, dummy);

        if (psum > total)
            hi = mid - 1;
        else
            lo = mid + 1;
    } while (lo <= hi);

    return (lo - 1);
}

/**
 * Interpolates between 2 bit allocations.
 *
 * @param[in]     bits_lo     Pointer to an array of per-band bit allocations for the lesser of the
 *                          2 q values that we plan to interpolate between.
 * @param[in]     bits_delta  adding bits_delta to bits_lo on a per-band basis will give bit
 *                            allocations for a q-value 1 above bits_lo.
 * @param[in]     thresh    Minimum number of bits for which PVQ shape will be encoded.
 * @param[in]     mid       fractional amount in fractions of 1/(2^alloc_bits) to interpolate above
 *                          bits_lo
 * @param[in]     alloc_steps
 * @param[out]    bits_out  returns the
 */
static int interpolate_fractional_q_between_bits(const frame_context_t* fc,
                                                 const int32_t* bits_lo, const int32_t* bits_delta,
                                                 const bit_allocation_parameters_t* bap,
                                                 int mid, int* bits_out)
{
    int j;
    int psum = 0;
    int done = 0;

    const int alloc_floor = fc->C << BITRES;
    const int alloc_steps = 6;

    for (j = 20; j >= 0; j--) {
        // Because we're doing a binary search over 64 integers, all of our divisions can just
        // be left-shifts. Only multiply here is by mid.
        int tmp = bits_lo[j] + ((mid * (int32_t)bits_delta[j]) >> alloc_steps);

        if ((tmp >= bap->pvq_threshold[j]) || done)
        {
            /* Don't allocate more than we can actually use */
            bits_out[j] = tmp;

            if (bits_out[j] > fc->cap[j])
                bits_out[j] = fc->cap[j];

            // All subsequent bands lower than this one get their bit allocation directly from the
            // interpolation
            done = 1;
        } else {
            // If this band and all bands higher than it have an interpolated bit allocation LOWER
            // than their PVQ threshold, we allocate either alloc_floor or 0 bits to those bands.
            // Once a band in descending order "breaks" this "rule", no subsequent, lower-freq bands
            // are eligable.
            if (tmp >= alloc_floor)
                bits_out[j] = alloc_floor;
            else
                bits_out[j] = 0;
        }

        psum += bits_out[j];
    }

    return psum;
}

/**
 * This function figures out how many bits each band gets
 *
 * int32_t total, int C, int LM, const int* thresh, const int* trim_offset
 */
static void calculate_bit_allocation_per_band(const frame_context_t* fc,
                                              const bit_allocation_parameters_t* bap,
                                              int32_t total,
                                              int32_t* result)
{
    int lo = search_q_lo(fc, bap, total);

    int bits_lo[21], bits_hi[21];
    calculate_bits_for_quality2(fc, bap, lo, bits_lo);
    calculate_bits_for_quality2(fc, bap, lo + 1, bits_hi);

    int bits_delta[21];
    for (int i = 0; i < 21; i++)
        bits_delta[i] = bits_hi[i] - bits_lo[i];

    // Binary search in increments of 1/64th for the value of q between lo and lo + 1 which
    // is as large as possible without exceeding the budget of 'total'.
    int lo_fractional = 0;
    int hi_fractional = 1 << 6;
    for (int i = 0; i < 6; i++) {
        int mid = (lo_fractional + hi_fractional) / 2;

        int dummy_bits[21] __attribute__((unused));
        int psum = interpolate_fractional_q_between_bits(fc, bits_lo, bits_delta, bap,
                                                         mid, dummy_bits);

        if (psum > total)
            hi_fractional = mid;
        else
            lo_fractional = mid;
    }

    printf("q_lo = %i\n", lo);
    printf("fractional q = %i / 64\n", lo_fractional);

    // Interpolate using the final value of lo_fractional that we found.
    interpolate_fractional_q_between_bits(fc, bits_lo, bits_delta, bap,
                                          lo_fractional, result);
}


bit_allocation_description_t* bit_allocation_create(const frame_context_t* fc,
                                                    range_decoder_t* rd)
{
    bit_allocation_description_t* ret = calloc(1, sizeof(bit_allocation_description_t));

    // Calculate band boosts
    bit_allocation_parameters_t* bit_allocation_parameters = bit_allocation_parameters_create(fc, rd);

    // total number of bits in 1/8th bits.
    // TODO: avoid calc duplications for bit reservations like anti_collapse_rsv
    //bits = (((opus_int32)len * 8) << BITRES) - ec_tell_frac(dec) - 1;
    //anti_collapse_rsv = isTransient && (LM >= 2) && (bits >= ((LM+2)<<BITRES)) ? (1 << BITRES) : 0;
    //bits -= anti_collapse_rsv;
    int32_t total = ((rd->len * 8) << BITRES) - range_decoder_tell_bits_fractional(rd) - 1;
    int anti_collapse_rsv = ((fc->transient &&
                             (fc->LM >= 2) &&
                             (total >= ((fc->LM + 2) << BITRES))) ?
                             (1 << BITRES) : 0);
    total -= anti_collapse_rsv;

    printf("computing allocation for % 5i 8th bits\n", total);

    // interpolate per-band bit allocations from q factor
    int32_t bits[21];
    calculate_bit_allocation_per_band(fc, bit_allocation_parameters, total, bits);

    printf("per-band bit allocation in 1/8th bits: ");
    for (int i = 0; i < 21; i++)
        printf("% 5ib, ", bits[i]);
    printf("\b\b \n");

    // decide which bands to skip
    // Looks like this searches up from the LF bands to find the band skip, but I'm not sure what
    // the rules are for determining band skip or the significance if it's set to 1.
    //
    // Seems like a frame with a single boost at a lower band (say band 3) would have skip_start
    // set to 3. Does that mean that all bands above band 3 are skipped??? That can't be right.
    // Seems like we will get more info once we look at interp_bits2pulses.
    //
    // Looking in interp_bits2pulses, it looks like band skipping starts at the top. Once the band
    // skipping loop in interp_bits2pulses reaches skip_start, it knows to stop skipping bands.
#if 0
    if (offsets[j]>0)
        skip_start = j;
#endif

    // intensity parameter (?)


    // distribute leftover bits


    // split allocated bits between fine quantization and shape

    return ret;
}
