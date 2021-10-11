#include "range_coder.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/**
 * Returns true if range decoder rd needs to be renormalized as described in 4.1.2.1.
 *
 * A range decoder needs renormalization if it's gotten too "zoomed in" on a certain range that
 * it's decoding.
 */
static bool range_decoder_needs_renormalization(range_decoder_t* rd)
{
    return (rd->rng <= (1 << 23));
}

/**
 * Decodes bits from the range decoder according to 4.1.2.1 until it's normalized or until it runs
 * out of bits - whichever comes first.
 *
 * An intuitive way of thinking about renormalization is that it's a "stretching and zooming in"
 * of the number line that's typically described in arithmetic encoding.
 *
 * @param[in,out] rd         Range decoder to renormalize.
 */
static void range_decoder_renormalize(range_decoder_t* rd)
{
    while (range_decoder_needs_renormalization(rd) && (rd->idx_front < rd->len)) {
        //printf("Renormalizing\n");

        const uint8_t byte_in = rd->data_front[rd->idx_front++];
        uint8_t newval = (rd->saved_lsb << 7) | (byte_in >> 1);
        rd->saved_lsb = byte_in & 0x01;

        rd->rng <<= 8;
        rd->val = ((rd->val << 8) + (255 - newval)) & 0x7fffffff;
    }
}

range_decoder_t* range_decoder_create(const uint8_t* data, int len)
{
    range_decoder_t* rd = calloc(sizeof(*rd), 1);

    rd->len = len;
    rd->data_front = data;
    rd->data_back  = data + (len - 1);

    // Initialize the range decoder state according to 4.1.1
    rd->rng = 128;
    uint8_t b0 = rd->data_front[rd->idx_front++];
    rd->val = 127 - (b0 >> 1);
    rd->saved_lsb = b0 & (1 << 0);

    // Load rd up so it's ready for it's first decoding.
    range_decoder_renormalize(rd);

    return rd;
}

uint32_t range_decoder_decode_symbol(range_decoder_t* rd,
                                     const symbol_context_t* symbol)
{
    //printf("Decoding symbol\n");
    print_symbol_context(symbol);

    // figure out where in the range the symbol lies
    uint32_t symbol_location = (rd->val / (rd->rng / symbol->ft)) + 1;
    uint32_t fs;
    if (symbol_location < symbol->ft) {
        fs = symbol->ft - symbol_location;
    } else {
        fs = 0;
    }

    // linear search over pdf values to find appropriate symbol
    bool symbol_found = false;
    uint32_t k = 0;
    for (int i = 0; i < symbol->num_symbols; i++) {
        if ((symbol->fl[i] <= fs) && (fs < symbol->fh[i])) {
            k = i;
            symbol_found = true;
            break;
        }
    }

    if (!symbol_found) {
        printf("Error: symbol not found for fs value %i\n", fs);
        assert(0);
    }

    //printf("fs:     %08x    k:     %08x\n", fs, k);

    // advance decoder state by scaling val and rng
    // this should be moved to its own function
    rd->val = rd->val - (rd->rng / symbol->ft) * (symbol->ft - symbol->fh[k]);

    if (symbol->fl[k] > 0) {
        rd->rng = (rd->rng / symbol->ft) * (symbol->fh[k] - symbol->fl[k]);
    } else {
        rd->rng = rd->rng - (rd->rng / symbol->ft) * (symbol->ft - symbol->fh[k]);
    }

    //printf("Decoded symbol: %i\n", k);

    range_decoder_renormalize(rd);

    return k;
}

uint32_t range_decoder_read_raw_bytes_from_back(range_decoder_t* rd, int n)
{
    const int32_t BITS_PER_BYTE = 8;

    // add first partial bit.
    int32_t bits_added = BITS_PER_BYTE - rd->back_bitidx;
    uint32_t ret = rd->data_back[-rd->idx_back] >> rd->back_bitidx;

    // add whole bytes until we exceed the # of bits we need.
    while (bits_added < n) {
        rd->idx_back++;
        ret |= (((uint32_t)rd->data_back[-rd->idx_back]) << bits_added);
        bits_added += 8;
    }

    // mask off additional bits
    uint32_t mask = ((uint32_t)1 << n) - 1;
    ret &= mask;

    // keep track of remaining balance.
    int32_t excess = bits_added - n;
    rd->back_bitidx = BITS_PER_BYTE - excess;

    return ret;
}


uint32_t range_decoder_read_uniform_integer(range_decoder_t* rd, uint32_t n)
{
    if (n > 255) {
        printf("range_decoder_read_uniform_integer: unimplemented\n");
        *((volatile int*)0) = 0xffeeffee;
        return -1;
    } else {
        // Decode the symbol
        uint32_t symbol_location = (rd->val / (rd->rng / n)) + 1;
        uint32_t symbol;

        if (symbol_location > n) {
            symbol = 0;
        } else {
            symbol = n - symbol_location;
        }

        // Update the decoder
        rd->val = rd->val - ((rd->rng / n) *  (n - (symbol + 1)));

        if (symbol > 0) {
            rd->rng = (rd->rng / n);
        } else {
            rd->rng = rd->rng - ((rd->rng / n) * (n - 1));
        }

        range_decoder_renormalize(rd);

        return symbol;
    }
}


void range_decoder_destroy(range_decoder_t* rd)
{
    free(rd);
}

void print_range_decoder_state(const range_decoder_t* rd)
{
    printf("val: %08x    rng: %08x\n", rd->val, rd->rng);
}

/**
 * Returns ceil(log2(x + 1)), so
 *     ilog2_ceil(4) --> 3
 *     ilog2_ceil(3) --> 2
 *     ilog2_ceil(9) --> 4
 */
static int32_t ilog2_ceil(uint32_t x)
{
    return 31 - __builtin_clz(x);
}

int32_t range_decoder_tell_bits(const range_decoder_t* rd)
{
    int32_t total_bits = (rd->idx_front * 8) + (rd->idx_back * 8) + rd->back_bitidx;
    total_bits -= ilog2_ceil(rd->rng);
    return total_bits;
}

/**
 * utility function that returns the fractional part of a floating point number.
 */
static double fpart(double n)
{
    return n - ((long)n);
}

int32_t range_decoder_tell_bits_fractional(const range_decoder_t* rd)
{
    int32_t total_bits = 8 * ((rd->idx_front * 8) + (rd->idx_back * 8) + rd->back_bitidx);

    // the most sensible implementation of this would just have
    //     ceil(log2(rng) * 8)
    // but we need to make it a little more convoluted than that to match up with the reference
    // implementation
    double bits_remaining_in_rng = 8 * ilog2_ceil(rd->rng) + floor(8. * fpart(log2(rd->rng)));
    total_bits -= (int32_t)(bits_remaining_in_rng);
    return total_bits;
}


void range_decoder_reserve_eighth_bits(range_decoder_t* rd, int32_t reserve_amount)
{
    rd->reserved_eighth_bits += reserve_amount;
}

/**
 * Decreases the number of reserved bits in rd by the indicated amount
 */
void range_decoder_dereserve_eighth_bits(range_decoder_t* rd, int32_t reserve_amount)
{
    rd->reserved_eighth_bits -= reserve_amount;
}

/**
 * Returns the number of unused and unreserved eighth bits in the range decoder.
 */
int32_t range_decoder_get_remaining_eighth_bits(const range_decoder_t* rd)
{
    return (((8 * rd->len) << BITRES) -
            range_decoder_tell_bits_fractional(rd) -
            rd->reserved_eighth_bits);

    //return ((8 * 8 * rd->len) - range_decoder_tell_bits_fractional(rd));
}
