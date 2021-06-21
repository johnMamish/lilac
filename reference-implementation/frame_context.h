#ifndef _FRAME_CONTEXT_H
#define _FRAME_CONTEXT_H

#include <stdint.h>
#include <stdbool.h>

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
} frame_context_t;

#endif
