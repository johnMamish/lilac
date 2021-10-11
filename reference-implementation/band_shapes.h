#ifndef _BAND_SHAPES_H
#define _BAND_SHAPES_H

#include <stdint.h>

/**
 * The length of this struct is implicitly given by the frame_context_t that it's associated with.
 */
typedef struct band_shape {
    /// How many partitions this one band is split up into.
    /// The length of each partition can be implicitly dervied from the frame_context_t that this
    /// struct is associated with.
    int n_partitions;

    /// collapse_mask[i] corresponds to partition i of the shape.
    /// A value of 0 indicates a collapsed vector; a value of 1 indicates an un-collapsed vector.
    uint8_t* collapse_mask;
    float** shape;

    /// If a band is split up into partitions, a relative gain must be encoded. This parameter
    /// holds the gain of the associated band shape.
    float gain;

    /// Number of 1/8th bits that the coder expected to use given S(L, K). Sometimes fewer bits
    /// than this value may be used in reality, but the subsequent steps of bit allocation depend
    /// on how many 1/8th bits were EXPECTED to be used instead of how many were actually used.
    int32_t expected_eighth_bit_consumption;
} band_shape_t;

/**
 * Basically a jagged 2-d array.
 *
 * TODO: maybe add information about collapse masks and band splitting / partitioning.
 */
typedef struct band_shapes {
    band_shape_t* shapes;
} band_shapes_t;

void band_shape_destroy(band_shape_t* bs);

#endif
