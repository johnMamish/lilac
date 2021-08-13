#include "frame_context.h"

#include "celt_defines.h"

#include <stdio.h>
#include <stdlib.h>

// Taken from celt/static_modes_{fixed,float}.h
// These are maximums in bits/sample.
static const int cache_caps50[168] = {
    // 2.5 ms frame
    // mono
    224, 224, 224, 224, 224, 224, 224, 224, 160, 160, 160, 160, 185, 185, 185, 178, 178, 168, 134, 61, 37,

    // stereo
    224, 224, 224, 224, 224, 224, 224, 224, 240, 240, 240, 240, 207, 207, 207, 198, 198, 183, 144, 66, 40,

    // 5ms frame
    160, 160, 160, 160, 160, 160, 160, 160, 185, 185, 185, 185, 193, 193, 193, 183, 183, 172, 138, 64, 38,

    240, 240, 240, 240, 240, 240, 240, 240, 207, 207, 207, 207, 204, 204, 204, 193, 193, 180, 143, 66, 40,

    // 10ms frame
    185, 185, 185, 185, 185, 185, 185, 185, 193, 193, 193, 193, 193, 193, 193, 183, 183, 172, 138, 65, 39,

    207, 207, 207, 207, 207, 207, 207, 207, 204, 204, 204, 204, 201, 201, 201, 188, 188, 176, 141, 66, 40,

    // 20ms frame
    193, 193, 193, 193, 193, 193, 193, 193, 193, 193, 193, 193, 194, 194, 194, 184, 184, 173, 139, 65, 39,

    204, 204, 204, 204, 204, 204, 204, 204, 201, 201, 201, 201, 198, 198, 198, 187, 187, 175, 140, 66, 40,
};

// Taken from
static const int band_boundaries_2500us[] = {
// 0  200  400  600  800   1k  1.2  1.4  1.6   2k  2.4  2.8
   0,   1,   2,   3,   4,   5,   6,   7,   8,  10,  12,  14,

//3.2   4k  4.8  5.6  6.8   8k  9.6  12k 15.6   end of 15.6 band
   16,  20,  24,  28,  34,  40,  48,  60,  78,  100
};

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
static void frame_context_initialize_band_boundaries(frame_context_t* fc)
{
    for (int i = 0; i <= NBANDS; i++) {
        fc->band_boundaries[i] = band_boundaries_2500us[i] << fc->LM;
    }
}


/**
 * Initializes fc->caps, an array holding per-band maximum bits per sample.
 *
 * Check out celt/celt.c:init_caps()
 *
 * @param[in,out] fc      frame_context_t whose capacities should be initialized. fc->LM, fc->C, and
 *                        fc->band_boundaries must already be filled out; fc->caps will be populated
 *                        according to these values.
 */
static void frame_context_initialize_caps(frame_context_t* fc)
{
    const int* lookup = &cache_caps50[NBANDS * ((2 * fc->LM) + fc->C - 1)];
    for (int i = 0; i < NBANDS; i++) {
        int N = fc->band_boundaries[i + 1] - fc->band_boundaries[i];
        fc->cap[i] = ((lookup[i] + 64) * fc->C * N) >> 2;
    }
}

frame_context_t* frame_context_create_and_read_basic_info(uint8_t toc, range_decoder_t* rd)
{
    frame_context_t* fc = calloc(1, sizeof(frame_context_t));

    fc->LM = (toc >> 3) & 0x03;
    fc->C = 1;

    frame_context_initialize_band_boundaries(fc);
    frame_context_initialize_caps(fc);

    // Decode 'static' symbols for frame context
    fc->silence = range_decoder_decode_symbol(rd, &CELT_silence_context);
    printf("\n");

    fc->post_filter = range_decoder_decode_symbol(rd, &CELT_post_filter_context);
    printf("\n");

    if (fc->post_filter) {
        printf("post-filter specified in frame but not implemented!\n");
        return NULL;
    }

    fc->transient = range_decoder_decode_symbol(rd, &CELT_transient_context);
    printf("\n");

    fc->intra = range_decoder_decode_symbol(rd, &CELT_intra_context);
    printf("\n");

    return fc;
}

void frame_context_destroy(frame_context_t* fc)
{
    free(fc);
}
