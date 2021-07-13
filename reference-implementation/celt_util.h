#ifndef _CELT_UTIL_H
#define _CELT_UTIL_H

#include <stdint.h>

#include "frame_context.h"

/**
 * Performs a table lookup into RFC6716 Tables 60 - 63.
 *
 * @param[in]
 * @param[in]     tf_select   tf_select value to use to select tf_change table
 * @param[in]     tf_changed  This value is the 0 / 1 at the top of tables 60 - 63.
 */
int32_t get_tf_change(const frame_context_t* ctx, bool tf_select, bool tf_changed);

#endif
