#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "fine_energy.h"

static float fine_raw_bits_to_db(int32_t raw_bits, int32_t nbits)
{
    return ((raw_bits + 0.5f) / ((float)(1 << nbits))) - .5f;
}

fine_energy_t* fine_energy_create_from_range_decoder(const frame_context_t* fc,
                                                     const bit_allocation_description_t* alloc,
                                                     range_decoder_t* rd)
{
    fine_energy_t* fe = calloc(1, sizeof(fine_energy_t));
    fe->energies = calloc(21, sizeof(float));

    for (int i = 0; i < 21; i++) {
        int32_t nbits = alloc->energy_bits[i];
        if (nbits >= 0) {
            int32_t fine_raw_bits = range_decoder_read_raw_bytes_from_back(rd, nbits);
            float fine_energy = fine_raw_bits_to_db(fine_raw_bits, nbits);
            fe->energies[i] = fine_energy;
        } else {
            fe->energies[i] = 0.f;
        }
    }

    return fe;
}

void fine_energy_destroy(fine_energy_t* fe)
{
    free(fe->energies);
    free(fe);
}
