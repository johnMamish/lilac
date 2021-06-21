#ifndef _RANGE_CODER_H
#define _RANGE_CODER_H

#include <stdint.h>

#include "symbol_context.h"

/**
 * Struct represting the state of a range decoder.
 */
typedef struct range_decoder {
    /**
     * These values determine the most recently decoded value and current "zoom-in" level for the
     * range decoder. They are described in section 4.1 of RFC6716.
     */
    uint32_t val, rng;

    ///
    uint32_t saved_lsb;

    /// Points to the start of the frame of raw bytes to be {en/de}coded by this range coder.
    const uint8_t* data_front;

    /// Points to the final byte of the frame of raw bytes to be {en/de}coded by this range coder.
    const uint8_t* data_back;

    /// Number of bytes in the frame that should be decoded.
    uint32_t len;

    /// Pointer describing how many bits in the current byte have been read from the back of the
    /// frame.
    uint32_t back_bitidx;

    /// Pointers describing how many bytes have been read from the front and back of the frame
    int32_t idx_front, idx_back;
} range_decoder_t;

/**
 * Creates a new range decoder to decode a specific frame of raw bytes.
 *
 * @param[in]     data       Frame of bytes that the created range decoder
 * @param[in]     len        length of frame to decode in bytes.
 *
 * @return Returns a newly constructed range decoder that should be freed with
 * range_decoder_destroy().
 */
range_decoder_t* range_decoder_create(const uint8_t* data, int len);

/**
 * Destroys a range decoder, freeing its memory.
 *
 * @param[in]     rd         Range decoder to destroy.
 */
void range_decoder_destroy(range_decoder_t* rd);

/**
 * Decodes one symbol from the range decoder, renormalizing at the end if necessary.
 *
 * @param[in,out] rd         Range decoder from which the symbol should be decoded. It should
 *                           be normalized before this function runs and will be normalized
 *                           before it returns.
 * @param[in]     symbol     Descriptor of symbol that needs to be decoded, mostly describing CDF.
 *
 * @return The symbol index that's decoded. For instance, given a symbol descriptor with a 6-element
 *         CDF, this function will return a value between 0 and 5, inclusive.
 */
uint32_t range_decoder_decode_symbol(range_decoder_t* rd,
                                     const symbol_context_t* symbol);

/**
 *
 */
void print_range_decoder_state(const range_decoder_t* s);

/**
 * Returns the number of bits left in the frame that's being decoded by rd.
 *
 * @param[in]     rd         Range decoder whose remaining number of bits we want to check.
 *
 * @return The number of bits left to be decoded in rd
 */
uint32_t range_decoder_tell_bits(const range_decoder_t* rd);

#endif
