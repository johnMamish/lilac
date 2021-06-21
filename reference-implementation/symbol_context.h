#ifndef _SYMBOL_CONTEXT_H
#define _SYMBOL_CONTEXT_H

#include <stdint.h>
#include "frame_context.h"

/**
 * Struct describing PDF of a symbol. For instance the symbol 'silence' in the CELT range encoding
 * has a PDF of {32767, 1} / 2. This means that it can take on 2 different values.
 */
typedef struct symbol_context {
    // number of symbols in this symbol context.
    uint32_t num_symbols;

    // string for debugging
    char* symbol_context_name;

    // ft holds the range of values that can be used to code this symbol.
    // For example, in the Opus RFC a PDF of {7, 1} / 8 would have an ft of 8.
    uint32_t  ft;

    /**
     * Dynamically allocated arrays holding values for fl and fh. These are derived from the
     * symbol context's PDF via the formula given in section 4.1 of RFC6716.
     * For example, a symbol context with a PDF of {5, 2, 1} / 8 would have fl and fh
     *     fl = {0, 5, 7}
     *     fh = {5, 7, 8}
     *
     * Each of these arrays has num_symbols elements; they need to be dyamically allocated.
     */
    uint32_t* fl;
    uint32_t* fh;
} symbol_context_t;


const extern symbol_context_t CELT_silence_context;
const extern symbol_context_t CELT_post_filter_context;
const extern symbol_context_t CELT_transient_context;
const extern symbol_context_t CELT_intra_context;

/**
 * This symbol tells the amount of energy in each band on a coarse level.
 *
 * Because both the encoder and the decoder have the same predictor state, only the prediction
 * error is encoded, presumably on a by-band basis.
 *
 * The PDF for the prediction error can be found in quant_bands.c in the official reference
 * implementation included with the RFC. There's a different PDF for inter- and intra- frames
 * and a different PDF for each frame size. In this example, we just use the PDF for 480 sample
 * intra- frames.
 */
const extern symbol_context_t CELT_coarse_energy_context;


// tf_change

// tf_select

// spread

// dyn. alloc.

// alloc. trim

// skip

// intensity

// dual

// fine energy

// residual

// anti-collapse

// finalize


/**
 * Creates a symbol context from the laplace distribution given a frame size, whether it's
 * inter or intra, and the band number.
 *
 * @param[in]     LM         LM is 0, 1, 2, or 3 depending on whether the frame size is 120, 240,
 *                           480, or 960 samples respectively.
 * @param[in]     intra      This value should be 0 for inter frames (frames that depend on previous
 *                           frames) and 1 for intra frames (standalone frames).
 * @param[in]     band_number  Value between 0 and 20 describing the band number.
 *
 * @return A new symbol context that
 */
symbol_context_t* symbol_context_create_from_laplace(const frame_context_t* context, uint32_t band_number);

/**
 * Frees the given symbol context and any associated storage.
 */
void symbol_context_destroy(symbol_context_t* symbol_context);

void print_symbol_context(const symbol_context_t* sym);

#endif
