#ifndef _PVQ_DECODER_H
#define _PVQ_DECODER_H

#include <stdint.h>

#include "pvq.h"

/**
 * Returns the maximum number of 1/8th bits that the encoder will use to encode a pyramid vector of
 * the indicated length 'L'.
 *
 * The pvq coder will choose the maximum value of K that it can given a specific vector length L
 * and specific bit-budget. For instance, if the coder has a bit budget of 150 1/8th bits and needs
 * to use that to encode a vector of length L = 8, it will use K = 9 'pulses' (the L1-norm of the
 * PVQ family it uses will be 9.) because it takes 146 1/8th bits to encode vectors in the family
 * S(L=8, K=9), but 154 1/8th bits to encode vectors in the family S(L=8, K=10) (see documentation
 * for 'get_eighth_bits_for_vector()' to see how these values are determined).
 *
 * However, the coder has 2 conditions that impose a maximum amount on the number of pulses that
 * it will use to encode a vector:
 *   - As described in RFC6716 section 4.3.4.4, each vector can take no more than 255 1/8th bits to
 *     encode. Values of K which result in a codebook which will take over 255 1/8th bits to encode
 *     for a given L are disallowed.
 *   - The reference implementation uses a lookup table to determine bit allocations for given
 *     S(L, K). This table does not have any entries for K > 128. For small L, K exceeding 128
 *     will still give familes S(L, K) that can be encoded with fewer than 255 1/8th bits, but
 *     these values of K are still disallowed because the reference implementation won't use
 *     them.
 *
 * If the number bits required is outside of the lookup table, it's still may be possible IN
 * PRINCIPLE to encode the entire band shape with a single PV, but the reference impl doesn't
 * do this, so we won't either.
 *
 * @param[in]     L         Length of the vector whose maximum allowed bit consumption we're
 *                          interested in.
 * @return Returns the maximum number of 1/8th bits that would be used to encode a vector of length
 *         L.
 */
int32_t get_max_eighth_bits_for_vector(const int32_t L);

/**
 * Returns the number of 1/8th bits it will take to encode a PV belonging to family S(L, K)
 *
 * The number returned will reflect the number that the reference implementation would've returned.
 * The reference implementation does a binary search over a fixed lookup table which has some gaps
 * in it,
 *
 * TODO: this function fails to round values of K as in the reference implementation.
 * TODO: this function returns the wrong value by one 1/8th bit for S(11, 9).
 *
 * Returns -1 for values of L that aren't supported; Only some values of L are expected because
 * of limitations on values that LM can take on.
 */
int32_t get_eighth_bits_for_pvq_family(const pvq_family_t* pvq_family);


#endif
