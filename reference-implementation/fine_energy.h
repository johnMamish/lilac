#ifndef _FINE_ENERGY_H
#define _FINE_ENERGY_H

#include "bit_allocation.h"
#include "frame_context.h"
#include "range_coder.h"

typedef struct fine_energy {
    /// Array of 21 values containing fine-energy quantization values.
    /// Once decoded, these values can be added to the previously derived coarse energy quantization
    /// values and then raised to the power of 2 to get final PVQ magnitude adjustments.
    float* energies;
} fine_energy_t;

/**
 * Decodes fine energy bits from a range decoder according to a given bit allocation. Puts the
 * decoded bits into a newly created fine_energy_t.
 */
fine_energy_t* fine_energy_create_from_range_decoder(const frame_context_t* fc,
                                                     const bit_allocation_description_t* alloc,
                                                     range_decoder_t* rd);

void fine_energy_destroy(fine_energy_t*);

#endif
