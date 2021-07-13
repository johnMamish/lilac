#ifndef _FRAME_CONTEXT_H
#define _FRAME_CONTEXT_H

#include <stdint.h>
#include <stdbool.h>

#include "celt_defines.h"

/**
 * On a per-frame basis, contains basic info about frames.
 *
 * The info in this struct comes from the TOC byte (section 3.1) or the statically coded symbols in
 * the frame (e.g. 'silence', 'post-filter', 'transient', etc...)
 */
typedef struct frame_context {
    /// Section 4.3.3 defines LM to be log2(frame_size/120).
    /// There are 4 valid CELT frame sizes: 120, 240, 480, and 960 samples. For each of these
    /// frame sizes, LM will be 0, 1, 2, and 3, respectively.
    uint32_t LM;

    /// Number of channels in the frame
    uint32_t C;

    /// Is this frame all silence?
    bool silence;

    /// Should the contents of this frame be post-filtered after MDCT? See RFC6716 section 4.3.7.1.
    bool post_filter;

    /// Does this frame contain multiple short MDCTs instead of a single long one? See RFC6716
    /// section 4.3.1 for more info.
    bool transient;

    /// Is this frame an intra frame (one that depends on previous frames for decoding)? See
    /// RFC6716 section 4.3.2.1.
    bool intra;

    /// Determines whether the decoded vector needs to be rotated. See RFC6716 section 4.3.4.3.
    int spread_decision;

    /// Holds the maximum bits per sample that can be allocated on a channel-by-channel basis.
    uint8_t cap[NBANDS];

    /// Contains the start and stop MDCT buckets for each of the pseudo-Bark critical bands.
    /// The values in eBands are determined by LM (which is determined by the number of samples in
    /// the frame) as well as by whether it's a short or long frame.
    /// e.g. band_boundaries[3] holds the MDCT bucket where the 3rd band starts.
    int band_boundaries[NBANDS];
} frame_context_t;

/**
 * Initializes fc->band_boundaries, an array holding the coefficient indicies of the start of each
 * band.
 *
 * Check out celt/modes.c:compute_ebands()
 *
 * @param[in,out] fc      frame_context_t whose band boundaries should be initialized. fc->LM and fc->C
 *                        must already be filled out; fc->band_boundaries will be populated according
 *                        to these values.
 */
void frame_context_initialize_band_boundaries(frame_context_t* fc);

/**
 * Initializes fc->caps, an array holding per-band maximum bits per sample.
 *
 * Check out celt/celt.c:init_caps()
 *
 * @param[in,out] fc      frame_context_t whose capacities should be initialized. fc->LM, fc->C, and
 *                        fc->band_boundaries must already be filled out; fc->caps will be populated
 *                        according to these values.
 */
void frame_context_initialize_caps(frame_context_t* fc);

#endif
