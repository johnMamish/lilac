#include "pvq.h"

#include "bit_allocation.h"
#include "pvq_decoder.h"
#include "band_shapes.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct pvq_decoder_state {

} pvq_decoder_state_t;

/**
 * Each band's shape is described by integer codewords corresponding to "Pyramid Vectors" - vectors
 * with integer components. "Pyramid Vectors" are grouped into families defined by a dimension - L -
 * and an L1-norm - K (a vector's L1-norm is found by adding together by removing minus signs from
 * its components and adding them all together). Each pyramid vector in a family is associated with
 * a specific integer codeword. RFC6716 section 4.3.4.4 limits the maximum size of PV families by
 * stating "the maximum size allowed for codebooks is 32 bits". This means that we can't encode PV's
 * belonging to families with 2^32 or more members. In this case, RFC6716 advises us that we should
 * break the band into 2 seperate sub-bands (called band partitions) and try again.
 *
 * This struct holds context needed to decode an individual band partition.
 */
typedef struct band_partition_context {
    int32_t LM;
    int32_t C;
    bool transient;
    int N;
} band_partition_context_t;

/**
 * Might want to move this function and the corresponding struct to frame_context.h
 *
 * The only thing that's holding me back is that a "band_partition" is strictly a pvq
 * decoder concept.
 *
 * @param[in]
 * @param[in]     band_number   Which band # should the returned band_partition_context take its
 *                              size from?
 */
static band_partition_context_t*
band_partition_context_create_from_frame_context(const frame_context_t* fc, int band_number)
{
    band_partition_context_t* bpc = calloc(1, sizeof(band_partition_context_t));

    bpc->LM        = fc->LM;
    bpc->C         = fc->C;
    bpc->transient = fc->transient;
    bpc->N         = (fc->band_boundaries[band_number + 1] -
                      fc->band_boundaries[band_number]);

    return bpc;
}

static void
band_partition_context_destroy(band_partition_context_t* bpc)
{
    free(bpc);
}

static pvq_decoder_state_t* pvq_decoder_state_create(bit_allocation_description_t* alloc) __attribute__((unused));
static pvq_decoder_state_t* pvq_decoder_state_create(bit_allocation_description_t* alloc)
{
    pvq_decoder_state_t* ret = NULL;

    return ret;
}

static void pvq_decoder_state_destroy(pvq_decoder_state_t* state) __attribute__((unused));
static void pvq_decoder_state_destroy(pvq_decoder_state_t* state)
{
    free(state);
}


static bool band_context_is_splittable(const band_partition_context_t* band_context)
{
    return ((band_context->LM != -1) && (band_context->N > 2));
}

int32_t get_max_eighth_bits_for_band_context(const band_partition_context_t* band_context)
{
    return get_max_eighth_bits_for_vector(band_context->N);
}

/**
 *
 */
static bool band_needs_split(const band_partition_context_t* band_context,
                             int32_t bit_budget)
{
    //const int32_t max_pvq_eighth_bits = 255;

    // We can't split a band partition whose LM is -1 or whose
    if (!band_context_is_splittable(band_context)) {
        return false;
    } else {
        // If we have a bigger budget than our PVQ encoder knows how to use, split the band up.
        int32_t max_eighth_bits = get_max_eighth_bits_for_band_context(band_context);
        const int32_t one_point_five_eighth_bits = 12;
        return (bit_budget > (max_eighth_bits + one_point_five_eighth_bits));
    }
}

/**
 *
 */
#if 0
static band_shape_t*
merge_partitions()
{
    return NULL;
}
#endif

/**
 * This function returns the maximum value that the symbol encoding theta can take on.
 */
static int32_t get_theta_range(const band_partition_context_t* band_context,
                               int32_t bit_budget,
                               range_decoder_t* rd)
{
    const int32_t QTHETA_OFFSET = 4;
    int32_t pulse_cap = (int32_t)ceil(8 * log2(band_context->N));
    int32_t offset = (pulse_cap / 2) - QTHETA_OFFSET;

    // exp2_table8[i] = floor(2^(14 + (1/8 * i)))
    static const int16_t exp2_table8[8] =
        {16384, 17866, 19483, 21247, 23170, 25267, 27554, 30048};

    // qb - number of 1/8th bits that will be used to encode qn.
    /* The upper limit ensures that in a stereo split with itheta==16384, we'll
       always have enough bits left over to code at least one pulse in the
       side; otherwise it would collapse, since it doesn't get folded. */
    int qb;
    qb = ((bit_budget + (offset * ((2 * band_context->N) - 1))) / ((2 * band_context->N) - 1));

    // theta shouldn't hog space from the actual Pyramid Vector
    if (qb > (bit_budget - pulse_cap - (4 << BITRES))) {
        qb = bit_budget - pulse_cap - (4 << BITRES);
    }

    // theta shouldn't exceed one byte.
    if (qb > (8 << BITRES)) {
        qb = (8 << BITRES);
    }

    int qn;
    if (qb < ((1 << BITRES) >> 1)) {
        // Make sure that qn is at least 1.
        qn = 1;
    } else {
        // qn = 2^(14 + (qb mod 8) / 8) / 2^(14 - qb / 8)
        qn = exp2_table8[qb&0x7]>>(14-(qb>>BITRES));

        // round up to the nearest multiple of 2. why? idk.
        // One reason that this might be here is that it ensures that qn is at least 1.
        qn = (qn+1)>>1<<1;
    }
    return qn;
}

static void calculate_bit_split_from_theta(const band_partition_context_t* bpc,
                                           int32_t theta,
                                           int32_t* eighth_bits)
{
    //int32_t delta = 0;

    eighth_bits[0] = theta * bpc->N;
    eighth_bits[1] = theta * bpc->N;
    /* Give more bits to low-energy MDCTs than they would otherwise deserve */
    #if 0
    if ((B0 > 1) && (itheta&0x3fff)) {
        if (itheta > 8192) {
            /* Rough approximation for pre-echo masking */
            delta -= delta >> (4 - band_context->LM);
        } else {
            /* Corresponds to a forward-masking slope of 1.5 dB per 10 ms */
            delta = IMIN(0, delta + (N<<BITRES>>(5-LM)));
        }
    }
    bits[0] = IMAX(0, IMIN(b, (b-delta)/2));
    #endif
}

/**
 * If a frame is transient, its band shapes will be broken up into 2^LM "short blocks".
 *
 * This function returns true if a given band partition context contains shapes for 2 different
 * MDCTs.
 */
static bool band_partition_context_contains_multiple_frames(const band_partition_context_t* bpc)
{
    return (bpc->transient && (bpc->LM > 0));
}

static int32_t decode_theta_uniform_pdf(const band_partition_context_t* bpc,
                                        int32_t bit_budget,
                                        range_decoder_t* rd)
{
    int32_t theta_max_symbol = get_theta_range(bpc, bit_budget, rd);
    int32_t theta_symbol = range_decoder_read_uniform_integer(rd, theta_max_symbol + 1);
    return (theta_symbol * 16384) / theta_max_symbol;
}

static int32_t decode_theta_triangular_pdf(const band_partition_context_t* bpc,
                                           int32_t bit_budget,
                                           range_decoder_t* rd)
{
    int32_t theta_max_symbol __attribute__((unused));
    theta_max_symbol = get_theta_range(bpc, bit_budget, rd);

    symbol_context_t* theta_symbol_context;
    theta_symbol_context = symbol_context_create_triangular_theta(theta_max_symbol);

    uint32_t itheta = range_decoder_decode_symbol(rd, theta_symbol_context);

    symbol_context_destroy(theta_symbol_context);

    return itheta;
}

/**
 * Whenever a band shape is split up into 2 different vectors, their relative magnitudes need to be
 * encoded. This is done via a single parameter called "theta". Theta is expressed in radians, but
 * is encoded as an uniformly distributed integer.
 *
 * @return Returns theta decoded from the bitstream. Theta is a value in [0, 16384). A value of
 *         0 for theta corresponds to 0 radians and a value of 16384 corresponds to pi / 2 radians.
 */
static int32_t decode_theta(const band_partition_context_t* bpc,
                            int32_t bit_budget,
                            range_decoder_t* rd)
{
    int32_t theta;
    int32_t theta_max_symbol = get_theta_range(bpc, bit_budget, rd);
    printf("theta qn: %5i\n", theta_max_symbol);

    if (band_partition_context_contains_multiple_frames(bpc)) {
        theta = decode_theta_uniform_pdf(bpc, bit_budget, rd);
    } else {
        theta = decode_theta_triangular_pdf(bpc, bit_budget, rd);
    }
    return theta;
}

static void calculate_band_gains_from_theta(const band_partition_context_t* bpc __attribute__((unused)),
                                            int32_t theta __attribute__((unused)),
                                            float* gains __attribute__((unused)))
{
    printf("calculate_band_gains_from_theta: UNIMPLEMENTED\n");
}

/**
 *
 *
 * What info do we need for this recursive function?
 *
 * @param[in]     band_context
 * @param[in]
 * @param[in,out] rd            Range decoder from which band shapes should be decoded.
 */
static band_shape_t*
band_shape_create_from_range_decoder_r(const band_partition_context_t* band_context,
                                       int32_t bit_budget,
                                       range_decoder_t* rd)
{
    band_shape_t* shape = NULL;

    //int32_t remaining_bits = total_bits - tell - 1;
    int32_t remaining_bits __attribute__((unused)) = range_decoder_get_remaining_eighth_bits(rd) - 1;
    int32_t range_coder_startpos __attribute__((unused)) = range_decoder_tell_bits_fractional(rd);
    if (band_needs_split(band_context, bit_budget)) {
        band_partition_context_t* subpartition_context = calloc(1, sizeof(band_partition_context_t));
        subpartition_context->C  = band_context->C;
        subpartition_context->N  = band_context->N >> 1;
        subpartition_context->LM = band_context->LM - 1;
        subpartition_context->transient = band_context->transient;

        // Decode theta
        int32_t theta = decode_theta(subpartition_context, bit_budget, rd);

        printf("decoded itheta: %3i\n", theta);
        fflush(stdout);

        // Calculate bit allocations and relative gains according to theta
        // Need total # of remaining bits less the bits taken by theta.
        static float gains[2];
        calculate_band_gains_from_theta(band_context, theta, gains);

        static int32_t eighth_bits[2];
        calculate_bit_split_from_theta(band_context, theta, eighth_bits);

        band_shape_t* lower = band_shape_create_from_range_decoder_r(subpartition_context,
                                                                     eighth_bits[0],
                                                                     rd);
        //rebalance_bits(eighth_bits);

        band_shape_t* upper = band_shape_create_from_range_decoder_r(subpartition_context,
                                                                     eighth_bits[1],
                                                                     rd);
        //shape = merge_partitions(lower, upper);

        band_shape_destroy(lower);
        band_shape_destroy(upper);
    } else {
        const int32_t LM __attribute__((unused)) = band_context->LM;

        band_shape_t* shape = calloc(1, sizeof(band_shape_t));
        shape->n_partitions = 1;
        shape->collapse_mask = calloc(1, sizeof(uint8_t));
        shape->shape = calloc(1, sizeof(float*));
        shape->shape[0] = calloc(band_context->N, sizeof(float));
        shape->gain = 1;

        //shape = decode_band_shape();

        shape->expected_eighth_bit_consumption = 0;
    }

    return shape;
}

/**
 * Tells how many bands the decoder should plan to use for the shape of a band.
 */
static int32_t get_provisional_bit_budget(const band_allocation_t* band_allocation,
                                          int32_t remaining_bands,
                                          const int32_t* balance,
                                          const range_decoder_t* rd)
{
    int32_t balance_split_param = remaining_bands;
    if (balance_split_param > 3)
        balance_split_param = 3;

    int32_t per_band_balance_share = *balance / balance_split_param;

    int32_t b = band_allocation->pvq_bits + per_band_balance_share;

    // boundaries for return value
    int32_t remaining_bits = range_decoder_get_remaining_eighth_bits(rd) - 1;
    if (b > (remaining_bits + 1))
        b = remaining_bits + 1;

    if (b > 16383)
        b = 16383;

    if (b < 0)
        b = 0;

    return b;
}

/**
 * What information does this function need, and what's the best way to organize that info?
 * I kind of want to just pass in a band number, but 'clean code' would probably tell me
 * "indexing is not the callee's job."
 *
 * Might be a good idea to have a, like, 'band descriptor' class that you can derive from
 * a frame context; "get me the 'band descriptor' for band 'i'".
 *
 * So - what info do we need?
 *     Bit allocation for this band only
 *     Number of remaining bits
 *
 *
 * (I think that) once we have the 1/8th bit allocation for the band in question, it doesn't
 * really change and we can do all of the calculation we need to do from that point.
 *
 * @param[in]     fc
 * @param[in]     band_allocation
 * @param[in]     remaining_bands  Number of bands left to decode (including the current one).
 * @param[in,out] balance          At entry should point to a variable holding the number of 1/8th
 *                                 bits left unused by the overall
 * @param[in,out] rd
 */
static band_shape_t* band_shape_create_from_range_decoder(const band_partition_context_t* bpc,
                                                          band_allocation_t* band_allocation,
                                                          int32_t remaining_bands,
                                                          int32_t* balance,
                                                          range_decoder_t* rd)
{
    int32_t tell_start = range_decoder_tell_bits_fractional(rd);

    int32_t bit_budget = get_provisional_bit_budget(band_allocation, remaining_bands, balance, rd);

    band_shape_t* shape;
    shape = band_shape_create_from_range_decoder_r(bpc, bit_budget, rd);   // do I need 'balance' for this function?

    // Adjust remaining bit budget to account for discrepancy between the number of bits we expected
    // to use vs the number of bits we actually used.
    int32_t eighth_bit_usage_actual = range_decoder_tell_bits_fractional(rd) - tell_start;
    int32_t eighth_bit_usage_expected = band_allocation->pvq_bits;
    int32_t eighth_bit_usage_excess = eighth_bit_usage_actual - eighth_bit_usage_expected;
    *balance -= eighth_bit_usage_excess;

    return shape;
}

band_shapes_t* decode_band_shapes(const frame_context_t* fc,
                                  bit_allocation_description_t* band_allocations,
                                  range_decoder_t* rd)
{
    //pvq_decoder_state_t* decoder_state = pvq_decoder_state_create(band_allocations);

    // for band b in bands
    //     adjust remaining bits
    //     allocate memory
    //     find bands to fold from
    //
    //
    for (int i = 0; i < 21; i++) {
        band_allocation_t* band_allocation = &band_allocations->bands[i];
        int32_t remaining_bands = 21 - i;
        band_partition_context_t* bpc = band_partition_context_create_from_frame_context(fc, i);
        band_shape_t* shape __attribute__((unused));
        shape = band_shape_create_from_range_decoder(bpc,
                                                     band_allocation,
                                                     remaining_bands,
                                                     &band_allocations->balance,
                                                     rd);

        // 'b', 'balance', 'remaining_bits'
        // adjust_balance(fc, band_allocation, rd);

        band_partition_context_destroy(bpc);
    }

    return NULL;
}
