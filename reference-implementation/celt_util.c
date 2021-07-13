#include "celt_util.h"

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
