#include "band_shapes.h"

#include <stdlib.h>

void band_shape_destroy(band_shape_t* bs)
{
    for (int i = 0; i < bs->n_partitions; i++) {
        free(bs->shape[i]);
    }
    free(bs->shape);

    free(bs->collapse_mask);

    free(bs);
}
