#ifndef _FRAME_CONTEXT_H
#define _FRAME_CONTEXT_H

#include <stdint.h>
#include <stdbool.h>

#include "celt_defines.h"

typedef struct post_filter_params {
    int32_t octave, period, gain, tapset;
} post_filter_params_t;

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
    int32_t LM;

    /// Number of channels in the frame
    /// signed integer because it's used in some calcs with other signed ints.
    int32_t C;

    /// Is this frame all silence?
    bool silence;

    /// Should the contents of this frame be post-filtered after MDCT? See RFC6716 section 4.3.7.1.
    bool post_filter;

    /// If post_filter is true, this struct contains decoded parameters about the post_filter
    post_filter_params_t post_filter_params;

    /// Does this frame contain multiple short MDCTs instead of a single long one? See RFC6716
    /// section 4.3.1 for more info.
    bool transient;

    /// Is this frame an intra frame (one that depends on previous frames for decoding)? See
    /// RFC6716 section 4.3.2.1.
    bool intra;

    /// Holds the maximum bits per sample that can be allocated on a channel-by-channel basis.
    int cap[NBANDS];

    /// Contains the start and stop MDCT buckets for each of the pseudo-Bark critical bands.
    /// The values in eBands are determined by LM (which is determined by the number of samples in
    /// the frame) as well as by whether it's a short or long frame.
    /// e.g. band_boundaries[3] holds the MDCT bucket where the 3rd band starts.
    int band_boundaries[NBANDS + 1];
} frame_context_t;

#include "range_coder.h"
#include "symbol_context.h"

/**
 * This function creates and initializes a new frame_context_t from the data at the start of an Opus
 * frame. The passed in range decoder 'rd' is advanced to the 'coarse energy' section of the
 * bitstream.
 *
 * @param[in]     toc     This should be the first byte in the frame.
 * @param[in,out] rd      Range decoder initialized to point to the second byte in the frame - the
 *                        one immediately following the TOC byte. On function return, rd will be
 *                        advanced to the point in the bitstream where the 'coarse energy' section
 *                        begins
 *
 * @return A newly-allocated frame_context_t that's been fully initialized according to the decoded
 *         data at the start of the frame.
 */
frame_context_t* frame_context_create_and_read_basic_info(uint8_t toc, range_decoder_t* rd);

void frame_context_destroy(frame_context_t* fc);

#endif
