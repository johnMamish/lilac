#ifndef _ANTI_COLLAPSE_H
#define _ANTI_COLLAPSE_H

#include "frame_context.h"
#include "range_coder.h"

/**
 * Modifies a range decoder's count of reserved bits so that - if appropriate - one bit for anti-
 * collapse is reserved.
 *
 */
void reserve_anti_collapse_bit_from_range_decoder(const frame_context_t* fc,
                                                  range_decoder_t* rd);

#endif
