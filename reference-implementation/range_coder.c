#include "range_coder.h"

#include <assert.h>
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
        printf("Renormalizing\n");

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
    printf("Decoding symbol\n");
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

    printf("fs:     %08x    k:     %08x\n", fs, k);

    // advance decoder state by scaling val and rng
    rd->val = rd->val - (rd->rng / symbol->ft) * (symbol->ft - symbol->fh[k]);

    if (symbol->fl[k] > 0) {
        rd->rng = (rd->rng / symbol->ft) * (symbol->fh[k] - symbol->fl[k]);
    } else {
        rd->rng = rd->rng - (rd->rng / symbol->ft) * (symbol->ft - symbol->fh[k]);
    }

    range_decoder_renormalize(rd);

    return k;
}

void range_decoder_destroy(range_decoder_t* rd)
{
    free(rd);
}

void print_range_decoder_state(const range_decoder_t* rd)
{
    printf("val: %08x    rng: %08x\n", rd->val, rd->rng);
}
