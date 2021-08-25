#ifndef _BIT_ALLOCATION_H
#define _BIT_ALLOCATION_H

#include <stdint.h>
#include "frame_context.h"

typedef struct bit_allocation_description {
    // bits allocated to fine energy
    int32_t energy_bits[21];

    // 1/8th bits allocated to pvq
    int32_t pvq_bits[21];

    int32_t fine_priority[21];

    // remaining bits
    int32_t balance;
} bit_allocation_description_t;

/**
 *
 * @param[in,out] rd         Should be pointing at the alloc. trim symbol.
 */
bit_allocation_description_t* bit_allocation_create(const frame_context_t* fc,
                                                    range_decoder_t* rd);

#endif
