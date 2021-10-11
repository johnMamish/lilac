#ifndef _BAND_SHAPE_DECODER_H
#define _BAND_SHAPE_DECODER_H

#include <stdint.h>

#include "band_shapes.h"
#include "bit_allocation.h"
#include "frame_context.h"
#include "pvq_decoder.h"
#include "pvq.h"
#include "range_coder.h"

/**
 * Decodes unit pyramid vectors from the range decoder according to the given bit allocation and
 * frame context.
 *
 * @param[in]     fc              Contains basic info about the frame.
 * @param[in,out] bit_allocation  Per-band bit allocation. As decoding happens, the actual per-band
 *                                bit allocation might differ from what was originally derived,
 *                                depending on which PVQ families are actually used and how the
 *                                remaining bit balance is used. The allocations will be updated to
 *                                reflect the actual decoded consumptions.
 * @param[in,out] rd              At the start of the band shape decoding process, the range decoder
 *                                should be pointing to the shape vectors in the bitstream.
 *
 * Note that some of the bands passed out of this function might be collapsed; a different function
 * needs to be responsible for folding collapsed bands.
 * Moreover, the returned bands might still be interleaved. De-interleaving needs to occur.
 */
band_shapes_t* decode_band_shapes(const frame_context_t* fc,
                                  bit_allocation_description_t* bit_allocation,
                                  range_decoder_t* rd);

#endif
