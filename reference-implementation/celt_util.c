#include "celt_util.h"
#include "opus_celt_macros.h"

#include <stdio.h>
#include <stdlib.h>

/* TF change table. Positive values mean better frequency resolution (longer
   effective window), whereas negative values mean better time resolution
   (shorter effective window). The second index is computed as:
   4*isTransient + 2*tf_select + per_band_flag */
const signed char tf_select_table[4][8] = {
    /*isTransient=0     isTransient=1 */
      {0, -1, 0, -1,    0,-1, 0,-1}, /* 2.5 ms */
      {0, -1, 0, -2,    1, 0, 1,-1}, /* 5 ms */
      {0, -2, 0, -3,    2, 0, 1,-1}, /* 10 ms */
      {0, -2, 0, -3,    3, 0, 1,-1}, /* 20 ms */
};

int32_t get_tf_change(const frame_context_t* ctx, bool tf_select, bool tf_changed)
{
    int transient_offset = ctx->transient ? 4 : 0;
    int tf_select_offset = tf_select ? 2 : 0;
    int tf_changed_offset = tf_changed ? 1 : 0;
    return tf_select_table[ctx->LM][transient_offset + tf_select_offset + tf_changed_offset];
}

/**
 * Reads the subsequent frame in fp into a newly allocated opus_raw_frame and returns it.
 *
 * returns NULL on failure.
 */
static opus_raw_frame_t* opus_raw_frame_create_from_file(FILE* fp)
{
    opus_raw_frame_t* f = calloc(1, sizeof(opus_raw_frame_t));

    if (fread(&f->length, sizeof(f->length), 1, fp) != 1) goto _exit_fail_2;
    if (fread(&f->rng_final, sizeof(f->rng_final), 1, fp) != 1) goto _exit_fail_2;

    f->length = __builtin_bswap32(f->length);
    f->rng_final = __builtin_bswap32(f->rng_final);

    if ((f->data = malloc(f->length)) == NULL) goto _exit_fail_2;

    if (fread(f->data, f->length, 1, fp) != 1) goto _exit_fail_1;

    return f;

_exit_fail_1:
    free(f->data);

_exit_fail_2:
    free(f);
    return NULL;
}

opus_raw_frame_t* opus_raw_frame_create_from_nth_in_file(const char* name, int frame_number)
{
    // open file
    FILE* fp;
    if ((fp = fopen(name, "rb")) == NULL) {
        return NULL;
    }

    // read frames until we reach the one we want.
    int i;
    for (i = 0; i < frame_number; i++) {
        opus_raw_frame_t* temp = opus_raw_frame_create_from_file(fp);
        opus_raw_frame_destroy(temp);
    }

    // read the frame we want
    opus_raw_frame_t* result = opus_raw_frame_create_from_file(fp);
    fclose(fp);

    return result;
}

void opus_raw_frame_destroy(opus_raw_frame_t* f)
{
    if (f) free(f->data);
    free(f);
}

/**
 * This code is not mine! It's taken directly from the Opus implementation.
 *
 * This is a cos() approximation designed to be bit-exact on any platform. Bit exactness
 * with this approximation is important because it has an impact on the bit allocation
 */
int16_t bitexact_cos(int16_t x)
{
    if (x <= 0) {
        return 32767;
    } else if (x >= 16384) {
        return 0;
    } else {
        int32_t tmp;
        tmp = (4096 + ((int32_t)(x)*(x))) >> 13;

        int16_t x2 = tmp;
        x2 = (32767 - x2) + FRAC_MUL16(x2, (-7651 + FRAC_MUL16(x2, (8277 + FRAC_MUL16(-626, x2)))));
        return 1 + x2;
    }
}

/**
 * This code is not mine! It's taken directly from the Opus implementation.
 */
int bitexact_log2tan(int isin, int icos)
{
    int lc;
    int ls;
    lc = EC_ILOG(icos);
    ls = EC_ILOG(isin);
    icos <<= (15 - lc);
    isin <<= (15 - ls);
    return (((ls - lc) * (1 << 11)) +
            FRAC_MUL16(isin, FRAC_MUL16(isin, -2597) + 7932) -
            FRAC_MUL16(icos, FRAC_MUL16(icos, -2597) + 7932));
}

int32_t restrict_to_range_int32(int32_t val, int32_t min, int32_t max)
{
    if (val > max) {
        return max;
    } else if (val < min) {
        return min;
    } else {
        return val;
    }
}
