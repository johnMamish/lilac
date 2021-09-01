#include "symbol_context.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// Log of the minimum probability of an energy delta
#define LAPLACE_LOG_MINP (0)

/// The minimum probability of an energy delta (out of 32768).
#define LAPLACE_MINP (1<<LAPLACE_LOG_MINP)

/**
 * The minimum number of guaranteed representable coarse energy deltas (in one direction).
 */
#define LAPLACE_NMIN (16)

const symbol_context_t CELT_silence_context = {
    .num_symbols = 2,
    .symbol_context_name = "silence symbol",
    .ft = 32768,
    .fl = (uint32_t[]) {0, 32767},
    .fh = (uint32_t[]) {32767, 32768}
};

const symbol_context_t CELT_post_filter_context = {
    .num_symbols = 2,
    .symbol_context_name = "post filter symbol",
    .ft = 2,
    .fl = ((uint32_t[]) {0, 1}),
    .fh = ((uint32_t[]) {1, 2})
};

const symbol_context_t CELT_transient_context = {
    .num_symbols = 2,
    .symbol_context_name = "transient",
    .ft = 8,
    .fl = ((uint32_t[]) {0, 7}),
    .fh = ((uint32_t[]) {7, 8})
};

const symbol_context_t CELT_intra_context = {
    .num_symbols = 2,
    .symbol_context_name = "intra",
    .ft = 8,
    .fl = ((uint32_t[]) {0, 7}),
    .fh = ((uint32_t[]) {7, 8})
};

const symbol_context_t CELT_spread_context = {
    .num_symbols = 4,
    .symbol_context_name = "spread decision symbol",
    .ft = 32,

    // 7, 2, 21, 2
    .fl = ((uint32_t[]) {0, 7, 9, 30}),
    .fh = ((uint32_t[]) {7, 9, 30, 32})
};

const symbol_context_t CELT_trim_context = {
    .num_symbols = 11,
    .symbol_context_name = "allocation trim symbol",
    .ft = 128,

    //                   2,  2,  5, 10, 22, 46, 22, 10,  5,  2,  2
    .fl = ((uint32_t[]) {0,  2,  4,  9, 19, 41, 87,109,119,124,126}),
    .fh = ((uint32_t[]) {2,  4,  9, 19, 41, 87,109,119,124,126,128}),
};

symbol_context_t* symbol_context_create(uint32_t num_symbols, uint32_t ft, const char* name)
{
    symbol_context_t* sc = calloc(1, sizeof(symbol_context_t));

    sc->num_symbols = num_symbols;
    sc->ft = ft;
    sc->symbol_context_name = strndup(name, 1024);

    sc->fl = calloc(num_symbols, sizeof(*sc->fl));
    sc->fh = calloc(num_symbols, sizeof(*sc->fh));

    return sc;
}

symbol_context_t* symbol_context_create_minprob_1(uint32_t logp, const char* name)
{
    uint32_t ft = (1 << logp);

    symbol_context_t* sc = symbol_context_create(2, ft, name);

    sc->fl[0] = 0;
    sc->fl[1] = ft - 1;
    sc->fh[0] = ft - 1;
    sc->fh[1] = ft;

    return sc;
}

symbol_context_t* symbol_context_create_equal_distribution_pow2(int32_t n, const char* name)
{
    if ((n > 0) && (n < 16)) {
        symbol_context_t* sc = symbol_context_create((1 << n), (1 << n), name);

        for (int i = 0; i < (1 << n); i++) {
            sc->fl[i] = i;
            sc->fh[i] = i + 1;
        }

        return sc;
    } else {
        return NULL;
    }
}

/**
 * Parameters of the Laplace-like probability models used for the coarse energy.
 * There is one pair of parameters for each frame size, prediction type
 * (inter/intra), and band number.
 * The first number of each pair is the probability of 0, and the second is the
 * decay rate, both in Q8 precision.
 */
static const uint8_t e_prob_model[4][2][42] = {
   /*120 sample frames.*/
   {
      /*Inter*/
      {
          72, 127,  65, 129,  66, 128,  65, 128,  64, 128,  62, 128,  64, 128,
          64, 128,  92,  78,  92,  79,  92,  78,  90,  79, 116,  41, 115,  40,
         114,  40, 132,  26, 132,  26, 145,  17, 161,  12, 176,  10, 177,  11
      },
      /*Intra*/
      {
          24, 179,  48, 138,  54, 135,  54, 132,  53, 134,  56, 133,  55, 132,
          55, 132,  61, 114,  70,  96,  74,  88,  75,  88,  87,  74,  89,  66,
          91,  67, 100,  59, 108,  50, 120,  40, 122,  37,  97,  43,  78,  50
      }
   },
   /*240 sample frames.*/
   {
      /*Inter*/
      {
          83,  78,  84,  81,  88,  75,  86,  74,  87,  71,  90,  73,  93,  74,
          93,  74, 109,  40, 114,  36, 117,  34, 117,  34, 143,  17, 145,  18,
         146,  19, 162,  12, 165,  10, 178,   7, 189,   6, 190,   8, 177,   9
      },
      /*Intra*/
      {
          23, 178,  54, 115,  63, 102,  66,  98,  69,  99,  74,  89,  71,  91,
          73,  91,  78,  89,  86,  80,  92,  66,  93,  64, 102,  59, 103,  60,
         104,  60, 117,  52, 123,  44, 138,  35, 133,  31,  97,  38,  77,  45
      }
   },
   /*480 sample frames.*/
   {
      /*Inter*/
      {
          61,  90,  93,  60, 105,  42, 107,  41, 110,  45, 116,  38, 113,  38,
         112,  38, 124,  26, 132,  27, 136,  19, 140,  20, 155,  14, 159,  16,
         158,  18, 170,  13, 177,  10, 187,   8, 192,   6, 175,   9, 159,  10
      },
      /*Intra*/
      {
          21, 178,  59, 110,  71,  86,  75,  85,  84,  83,  91,  66,  88,  73,
          87,  72,  92,  75,  98,  72, 105,  58, 107,  54, 115,  52, 114,  55,
         112,  56, 129,  51, 132,  40, 150,  33, 140,  29,  98,  35,  77,  42
      }
   },
   /*960 sample frames.*/
   {
      /*Inter*/
      {
          42, 121,  96,  66, 108,  43, 111,  40, 117,  44, 123,  32, 120,  36,
         119,  33, 127,  33, 134,  34, 139,  21, 147,  23, 152,  20, 158,  25,
         154,  26, 166,  21, 173,  16, 184,  13, 184,  10, 150,  13, 139,  15
      },
      /*Intra*/
      {
          22, 178,  63, 114,  74,  82,  84,  83,  92,  82, 103,  62,  96,  72,
          96,  67, 101,  73, 107,  72, 113,  55, 118,  52, 125,  52, 118,  52,
         117,  55, 135,  49, 137,  39, 157,  32, 145,  29,  97,  33,  77,  40
      }
   }
};


void print_symbol_context(const symbol_context_t* sym)
{
    //return;
    printf("Decoding symbol: %s\n", sym->symbol_context_name);
    printf("PDF: {");
    for (int i = 0; i < sym->num_symbols; i++) {
        printf("%i", sym->fh[i] - sym->fl[i]);
        if (i != (sym->num_symbols - 1)) printf(", ");
    }
    printf("} / %i\n", sym->ft);
}

/* The minimum probability of an energy delta (out of 32768). */
#define LAPLACE_LOG_MINP (0)
#define LAPLACE_MINP (1<<LAPLACE_LOG_MINP)
/* The minimum number of guaranteed representable energy deltas (in one
    direction). */
#define LAPLACE_NMIN (16)

static const uint32_t q15_1 = (1 << 15);
static const uint32_t q14_1 = (1 << 14);

symbol_context_t* symbol_context_create_from_laplace(const frame_context_t* context, uint32_t band_number)
{
    symbol_context_t* sc = calloc(sizeof(*sc), 1);

    // e_prob_model is a fixed table containing laplace distribution parameters for coarse energy
    // symbols as described in rfc 6716 section 4.3.2.1.
    int intra = context->intra ? 1 : 0;
    const uint32_t q15_probability_0 = e_prob_model[context->LM][intra][band_number * 2] << 7;
    const uint32_t q14_decay = e_prob_model[context->LM][intra][band_number * 2 + 1] << 6;

    sc->num_symbols = 0;
    sc->symbol_context_name = malloc(64);
    snprintf(sc->symbol_context_name, 64, "Laplace symbol: fs = % 5i; decay = % 5i",
             q15_probability_0, q14_decay);

    // Allocate memory for fl and fh. To simplify code, just allocate way more than we need.
    sc->ft = (1 << 15);
    sc->fl = calloc(sizeof(*sc->fl), 32768);
    sc->fh = calloc(sizeof(*sc->fh), 32768);

    // Fill out probability distribution.
    uint16_t q15_probability_accum = 0;

    // start with the probability of a 0.
    q15_probability_accum += q15_probability_0;
    sc->fh[0] = q15_probability_accum;
    sc->num_symbols++;

    // initialize q15_probability_val to be the probability of a +/-1 before the start of the loop.
    const uint32_t q15_tail_probability = LAPLACE_MINP * LAPLACE_NMIN;
    uint32_t q15_probability_val = (((q15_1 - (2 * q15_tail_probability) - q15_probability_0) *
                                     (q14_1 - q14_decay)) / 2) >> 14;

    do {
        // record next 2 values in distribution
        q15_probability_accum += q15_probability_val + LAPLACE_MINP;
        sc->fh[sc->num_symbols + 0] = q15_probability_accum;
        q15_probability_accum += q15_probability_val + LAPLACE_MINP;
        sc->fh[sc->num_symbols + 1] = q15_probability_accum;
        sc->num_symbols += 2;

        // advance the probability value to the next symbol.
        q15_probability_val = (q15_probability_val * q14_decay) >> 14;
    } while (q15_probability_val >= LAPLACE_MINP);

    // now do some minp buckets
    while (1) {
        if ((q15_probability_accum + (2 * LAPLACE_MINP)) > 32768) {
            break;
        }

        q15_probability_accum += LAPLACE_MINP;
        sc->fh[sc->num_symbols + 0] = q15_probability_accum;
        q15_probability_accum += LAPLACE_MINP;
        sc->fh[sc->num_symbols + 1] = q15_probability_accum;
        sc->num_symbols += 2;
    }

    // fill out fl
    sc->fl[0] = 0;
    for (int i = 1; i < sc->num_symbols; i++) {
        sc->fl[i] = sc->fh[i - 1];
    }

    assert(sc->num_symbols >= (2 * (LAPLACE_NMIN + 1)) + 1);

    return sc;
}


void symbol_context_destroy(symbol_context_t* sc)
{
    free(sc->symbol_context_name);
    free(sc->fl);
    free(sc->fh);

    free(sc);
}
