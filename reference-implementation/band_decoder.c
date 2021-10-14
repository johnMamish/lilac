#include "pvq.h"

#include "band_shapes.h"
#include "bit_allocation.h"
#include "celt_util.h"
#include "opus_celt_macros.h"
#include "pvq_decoder.h"

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

static char recursion_padding[65] = "";

static void recursion_string_increase_depth(char* s)
{
    int len = strlen(s);
    int i = 0;
    for (i = 0; i < (len + 4); s[i++] = ' ');
    s[i] = '\0';
}


static void recursion_string_decrease_depth(char* s)
{
    int len = strlen(s);
    s[len - 4] = '\0';
}


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
 * Returns a newly allocated band
 */
static band_shape_t*
merge_partitions(band_shape_t* first, band_shape_t* second)
{
    band_shape_t* shape = calloc(1, sizeof(band_shape_t));

    shape->n_partitions = first->n_partitions + second->n_partitions;
    shape->collapse_mask = calloc(shape->n_partitions, sizeof(uint8_t));

    int i;
    shape->shape = calloc(shape->n_partitions, sizeof(float*));
    for (i = 0; i < first->n_partitions; i++) {
        // TODO actually copy shape.
        shape->shape[i] = NULL;
    }
    for (; i < (first->n_partitions + second->n_partitions); i++) {
        // TODO actually copy shape.
        shape->shape[i] = NULL;
    }

    // TODO: figure out right value
    shape->gain = 1;

    shape->expected_eighth_bit_consumption = (first->expected_eighth_bit_consumption +
                                              second->expected_eighth_bit_consumption);

    return shape;
}

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
    int32_t itheta = range_decoder_read_uniform_integer(rd, theta_max_symbol + 1);
    printf("%sdecoded itheta: %3i\n", recursion_padding, itheta);
    return (itheta * 16384) / theta_max_symbol;
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

    printf("%sdecoded itheta: %3i\n", recursion_padding, itheta);

    return (itheta * 16384) / theta_max_symbol;
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
    printf("%stheta qn: %5i\n", recursion_padding, theta_max_symbol);

    if (band_partition_context_contains_multiple_frames(bpc)) {
        theta = decode_theta_uniform_pdf(bpc, bit_budget, rd);
    } else {
        theta = decode_theta_triangular_pdf(bpc, bit_budget, rd);
    }
    return theta;
}

/**
 * This function calculates the relative gain of two adjacently coded band shapes from the theta
 * parameter.
 */
static void calculate_band_gains_from_theta(const band_partition_context_t* bpc,
                                            int32_t theta,
                                            float gains[2])
{
    int igains[2] = {bitexact_cos((int16_t)theta), bitexact_cos((int16_t)(16384 - theta))};
    gains[0] = (1.f / 32768) * igains[0];
    gains[1] = (1.f / 32768) * igains[1];
}

/**
 *
 */
static void calculate_bit_split_from_theta(const band_partition_context_t* bpc,
                                           int32_t bit_budget,
                                           int32_t theta,
                                           int32_t eighth_bits[2])
{
    // delta is a parameter which determines the difference
    int32_t delta;
    if (theta == 0) {
        delta = -16384;
    } else if (theta == 16384) {
        delta = 16384;
    } else {
        int igains[2] = {bitexact_cos((int16_t)theta), bitexact_cos((int16_t)(16384 - theta))};
        delta = FRAC_MUL16((bpc->N - 1) << 7, bitexact_log2tan(igains[1], igains[0]));
    }

    // TODO: does this conditional correctly detect when to add pre/post masking adjustment?
    if (band_partition_context_contains_multiple_frames(bpc)) {
        // If the 2 sub-blocks belong to a transient frame and therefore correspond to different
        // time ranges, one might temporally mask the other. For instance, if the sub-block that
        // occurs first has a higher magnitude than the one that occurs second, it will temporally
        // mask the second, quieter one; the human ear will be less sensitive to detail in the
        // second block.
        // Further adjustments are made to delta to account for temporal masking.
        if (theta > 8192) {
            // delta for the temp
            delta -= delta >> (4 - bpc->LM);
        } else {
            // If the temporally later sub-block has lower magnitude, forward-masking will occur.
            // we can
            delta = delta + ((bpc->N << BITRES) >> (5 - bpc->LM));
        }
    }

    eighth_bits[0] = restrict_to_range_int32((bit_budget - delta) / 2, 0, bit_budget);
    eighth_bits[1] = bit_budget - eighth_bits[0];


    printf("%sdelta: %5i, mbits: %3i, sbits: %3i\n",
           recursion_padding, delta, eighth_bits[0], eighth_bits[1]);
}

/**
 *
 */
static void rebalance_bits(int32_t eighth_bits[2],
                           int32_t theta,
                           const band_shape_t* newly_decoded_band)
{
    int32_t bits_unused_by_first_band = (eighth_bits[0] -
                                         newly_decoded_band->expected_eighth_bit_consumption);

    if ((bits_unused_by_first_band > (3 << BITRES)) && (theta != 0)) {
        eighth_bits[1] += bits_unused_by_first_band - (3 << BITRES);
    }
}

static int get_pulses(int i)
{
    if (i < 8) {
        return i;
    } else {
        return (8 + (i % 8)) << ((i / 8) - 1);
    }
}

static int32_t calculate_eighth_bit_consumption_for_pvq_family(const pvq_family_t* f)
{
    uint64_t size = pvq_family_calculate_size(f);
    float ebits = 8 * log2(size);
    int32_t ebitsi = (int32_t)floor(ebits - 1e-6);

    return ebitsi;
}

/**
 * Determines the max number of pulses that can be encoded with a specific 1/8th-bit budget.
 *
 * TODO: right now this does a linear search involving a SUPER SUPER expensive calculation which
 * can be done at compile time. Need to move that back to compile time.
 *
 */
static pvq_family_t* determine_appropriate_pvq_family(const band_partition_context_t* bpc,
                                                      int32_t bit_budget)
{
    pvq_family_t f = {.l = bpc->N, .k = 0};

    int32_t highest_k = 0;

    for (int i = 1; i < 41; i++) {
        f.k = get_pulses(i);
        //printf("calculate_k: evaluating PVQ family {.l = %i, .k = %i}\n", f.l, f.k);

        int32_t ebitsi = calculate_eighth_bit_consumption_for_pvq_family(&f);

        if ((ebitsi + 1) > bit_budget) {
            break;
        } else {
            highest_k = f.k;
        }
    }

    pvq_family_t* ret = calloc(1, sizeof(pvq_family_t));
    ret->l = bpc->N;
    ret->k = highest_k;
    return ret;
}

static band_shape_t* decode_block_shape(const band_partition_context_t* bpc,
                                        range_decoder_t* rd,
                                        int32_t eighth_bit_budget)
{
    band_shape_t* shape = calloc(1, sizeof(band_shape_t));

    shape->n_partitions = 1;
    shape->collapse_mask = calloc(1, sizeof(uint8_t));
    shape->shape = calloc(1, sizeof(float*));
    shape->shape[0] = calloc(bpc->N, sizeof(float));
    shape->gain = 1;

    // Given our bit budget and vector length, what's the appropriate PVQ family to use?
    // Throw an extra eighth bit in there for conservativeness?
    pvq_family_t* pvq_family = determine_appropriate_pvq_family(bpc, eighth_bit_budget);
    shape->expected_eighth_bit_consumption = calculate_eighth_bit_consumption_for_pvq_family(pvq_family) + 1;

    printf("%sdecoding PVQ family: {.l = %3i, .k = %3i} from %5i eighth bits; budget %5i eighth bits\n",
           recursion_padding,
           pvq_family->l, pvq_family->k, shape->expected_eighth_bit_consumption, eighth_bit_budget);

    uint32_t size = pvq_family_calculate_size(pvq_family);
    range_decoder_read_uniform_integer(rd, size);

    free(pvq_family);

    return shape;
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
    recursion_string_increase_depth(recursion_padding);
    printf("%sstart LM = %i, budget = %3i\n", recursion_padding, band_context->LM, bit_budget);

    band_shape_t* shape = NULL;

    //int32_t remaining_bits = total_bits - tell - 1;
    int32_t remaining_bits __attribute__((unused)) = range_decoder_get_remaining_eighth_bits(rd) - 1;
    int32_t range_coder_startpos __attribute__((unused)) = range_decoder_tell_bits_fractional(rd);
    if (band_needs_split(band_context, bit_budget)) {
        // TODO: split this into a function
        band_partition_context_t* subpartition_context = calloc(1, sizeof(band_partition_context_t));
        subpartition_context->C  = band_context->C;
        subpartition_context->N  = band_context->N >> 1;
        subpartition_context->LM = band_context->LM - 1;
        subpartition_context->transient = band_context->transient;

        // Decode theta
        int32_t tell = range_decoder_tell_bits_fractional(rd);
        int32_t theta = decode_theta(subpartition_context, bit_budget, rd);
        int32_t ebits_used_by_theta = range_decoder_tell_bits_fractional(rd) - tell;
        printf("%sqalloc = %3i\n", recursion_padding, ebits_used_by_theta);
        bit_budget -= ebits_used_by_theta;

        fflush(stdout);

        // Calculate bit allocations and relative gains according to theta
        float mid_side_gains[2];
        calculate_band_gains_from_theta(subpartition_context, theta, mid_side_gains);

        int32_t mid_side_bit_allocations[2];
        calculate_bit_split_from_theta(subpartition_context, bit_budget, theta, mid_side_bit_allocations);

        // We always decode the sub-block with more bits first.
        bool swap_subblocks = (mid_side_bit_allocations[1] > mid_side_bit_allocations[0]);
        if (swap_subblocks) {
            int32_t t = mid_side_bit_allocations[1];
            mid_side_bit_allocations[1] = mid_side_bit_allocations[0];
            mid_side_bit_allocations[0] = t;
        }

        band_shape_t* first_band;
        band_shape_t* second_band;
        first_band = band_shape_create_from_range_decoder_r(subpartition_context,
                                                            mid_side_bit_allocations[0],
                                                            rd);
        rebalance_bits(mid_side_bit_allocations, theta, first_band);

        second_band = band_shape_create_from_range_decoder_r(subpartition_context,
                                                             mid_side_bit_allocations[1],
                                                             rd);

        if (swap_subblocks) {
            band_shape_t* temp = first_band;
            first_band = second_band;
            second_band = temp;
        }

        shape = merge_partitions(first_band, second_band);

        // HACK: should probably find a way to move this into merge_partitions()
        shape->expected_eighth_bit_consumption += ebits_used_by_theta;

        band_shape_destroy(first_band);
        band_shape_destroy(second_band);
    } else {
        shape = decode_block_shape(band_context, rd, bit_budget);
    }

    printf("%send LM = %i\n", recursion_padding, band_context->LM);
    recursion_string_decrease_depth(recursion_padding);

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
