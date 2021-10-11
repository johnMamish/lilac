#include <stdint.h>

#include "anti_collapse.h"
#include "range_coder.h"

/**
 * RFC 6716 section 4.3.3 describes conditions under which an anti-collapse bit is reserved:
 * For 10 ms and 20 ms frames using short blocks and that have at least
 * LM+2 bits left prior to the allocation process, one anti-collapse bit
 * is reserved in the allocation process so it can be decoded later.
 */
static bool anti_collapse_bit_should_be_reserved(const frame_context_t* fc,
                                                 const range_decoder_t* rd)
{
    // Opus subtracts 1/8th bit from the given total for a conservative estimate.
    int32_t total_eighth_bits = range_decoder_get_remaining_eighth_bits(rd) - 1;
    return (fc->transient &&
            (fc->LM >= 2) &&
            (total_eighth_bits >= ((fc->LM + 2) << BITRES)));
}

void reserve_anti_collapse_bit_from_range_decoder(const frame_context_t* fc,
                                                  range_decoder_t* rd)
{
    if (anti_collapse_bit_should_be_reserved(fc, rd)) {
        range_decoder_reserve_eighth_bits(rd, (1 << BITRES));
    }
}
