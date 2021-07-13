#include "frame_context.h"

#include "celt_defines.h"

// Taken from celt/static_modes_{fixed,float}.h
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


void frame_context_initialize_band_boundaries(frame_context_t* fc)
{
    for (int i = 0; i <= NBANDS; i++) {
        fc->band_boundaries[i] = band_boundaries_2500us[i] << fc->LM;
    }
}


void frame_context_initialize_caps(frame_context_t* fc)
{
    const int* lookup = &cache_caps50[NBANDS * ((2 * fc->LM) + fc->C - 1)];
    for (int i = 0; i < NBANDS; i++) {
        int N = fc->band_boundaries[i + 1] - fc->band_boundaries[i];
        fc->cap[i] = ((lookup[i] + 64) * fc->C * N) >> 2;
    }
}
