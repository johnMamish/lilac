#ifndef _CELT_UTIL_H
#define _CELT_UTIL_H

#include <stdint.h>

#include "frame_context.h"

typedef struct opus_raw_frame {
    uint32_t length;
    uint32_t rng_final;
    uint8_t* data;
} opus_raw_frame_t;

/**
 * Performs a table lookup into RFC6716 Tables 60 - 63.
 *
 * @param[in]
 * @param[in]     tf_select   tf_select value to use to select tf_change table
 * @param[in]     tf_changed  This value is the 0 / 1 at the top of tables 60 - 63.
 */
int32_t get_tf_change(const frame_context_t* ctx, bool tf_select, bool tf_changed);

/**
 * Reads the n-th frame from an opus file output by opus_demo -e.
 *
 * When opus_demo encodes pcm files, the output opus frames are output in a makeshift file format.
 * Frames are coded back-to-back where each frame consists of the following (big-endian) fields:
 *
 *     uint32_t frame_length;
 *     uint32_t final_rng_value;
 *     uint8_t  bytes[frame_length];
 *
 * Given the filepath of a binary file output by opus_demo, this function returns a newly-
 * allocated array containing the bytes of the frame.
 *
 * @param[in]     name          Filename to be opened.
 * @param[in]     frame_number  Frame number (0 indexed) to read.
 * @param[out]    data          After this function executes, opus_raw_frame will
 *
 * @return Returns a new opus_raw_frame on success, returns NULL on failure.
 */
opus_raw_frame_t* opus_raw_frame_create_from_nth_in_file(const char* name, int frame_number);

void opus_raw_frame_destroy(opus_raw_frame_t* f);

/**
 * These functions need to be implemented to be bit-exact, and the ones in the official Opus
 * implementation are correct by definition, so we just take those.
 */
int16_t bitexact_cos(int16_t x);
int bitexact_log2tan(int isin, int icos);

/**
 * Restricts val so that it lies within the range [min, max].
 */
int32_t restrict_to_range_int32(int32_t val, int32_t min, int32_t max);

#endif
