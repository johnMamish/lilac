#ifndef _BIT_ALLOCATION_H
#define _BIT_ALLOCATION_H

#include <stdint.h>
#include "frame_context.h"

typedef struct band_allocation {
    // bits allocated to fine energy for this band
    int32_t energy_bits;

    // 1/8th bits allocated to pvq for this band
    int32_t pvq_bits;

    // fine priority; a value of 0 or 1.
    int32_t fine_priority;
} band_allocation_t;

typedef struct bit_allocation_description {
    band_allocation_t bands[21];

    // Remaining bits at the end of 'band_allocations' not used by any of the bands.
    int32_t balance;
} bit_allocation_description_t;

/**
 *
 * @param[in,out] rd         Should be pointing at the alloc. trim symbol.
 */
bit_allocation_description_t* bit_allocation_create(const frame_context_t* fc,
                                                    range_decoder_t* rd);

#endif
