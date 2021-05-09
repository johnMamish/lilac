/**
 * Decodes a single CELT frame and dumps human-readable information about it to stdout.
 */

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>


#define ARRAY_NUMEL(arr) (sizeof(arr) / (sizeof(arr[0])))

/**
 * Struct describing PDF of a symbol. For instance the symbol 'silence' in the CELT range encoding
 * has a PDF of {32767, 1} / 2. This means that it can take on 2 different values.
 */
typedef struct symbol_context {
    // number of symbols in this symbol context.
    uint32_t num_symbols;

    // string for debugging
    const char* symbol_context_name;

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

/**
 *
 */
typedef struct range_decoder_state {
    uint32_t val, rng, saved_lsb;
} range_decoder_state_t;


const symbol_context_t CELT_silence_context = {
    .num_symbols = 2,
    .symbol_context_name = "silence symbol",
    .ft = 32768,
    .fl = (int[]) {0, 32767},
    .fh = (int[]) {32767, 32768}
};

const symbol_context_t CELT_post_filter_context = {
    .num_symbols = 2,
    .symbol_context_name = "silence symbol",
    .ft = 2,
    .fl = ((int[]) {0, 1}),
    .fh = ((int[]) {1, 2})
};

const symbol_context_t CELT_transient_context = {
    .num_symbols = 2,
    .symbol_context_name = "transient",
    .ft = 8,
    .fl = ((int[]) {0, 7}),
    .fh = ((int[]) {7, 8})
};

const symbol_context_t CELT_intra_context = {
    .num_symbols = 2,
    .symbol_context_name = "intra",
    .ft = 8,
    .fl = ((int[]) {0, 7}),
    .fh = ((int[]) {7, 8})
};

const symbol_context_t CELT_coarse_energy_context = {
    .num_symbols = 2,
    .symbol_context_name = "???",
    .ft = 2,
    .fl = ((int[]) {0, 1}),
    .fh = ((int[]) {1, 2})
};


#if 0
const symbol_context_t CELT_transient_context = {
    .num_symbols = ,
    .symbol_context_name = ,
    .ft = ,
    .fl = ((int[]) {}),
    .fh = ((int[]) {})
};
#endif

const symbol_context_t* symbols_to_decode[] = {
    &CELT_silence_context,
    &CELT_post_filter_context,
    &CELT_transient_context,
    &CELT_intra_context,
};


/**
 * Returns 1 if the range decoder state needs to be renormalized. Criteria are described in RFC6716
 * section 4.1.2.1.
 */
int range_decoder_state_needs_renormalization(const range_decoder_state_t* s);

/**
 * Renormalizes the range decoder state by consuming a byte from input.
 * The caller is responsible for removing the byte from the input stream.
 *
 * This function assumes that renormalization is required; if it's called without renorm being
 * required, the input byte passed in will be lost.
 */
void range_decoder_state_renormalize(range_decoder_state_t* s, uint8_t byte_in);


/**
 *
 */
uint32_t range_decoder_state_decode_symbol(range_decoder_state_t* s,
                                           const symbol_context_t* sym);


void print_range_decoder_state(const range_decoder_state_t* s);
void print_symbol_context(const symbol_context_t* sym);

uint8_t frame[] =
{
    0x5f, 0x48, 0x95, 0x5a, 0x27, 0x38, 0x72,
    0x17, 0xe0, 0xc2, 0x34, 0x30, 0x13, 0x37, 0x82,
    0x63, 0x2d, 0xc4, 0x26, 0x75, 0xce, 0xb3, 0x4d,
    0x72, 0x02, 0x31, 0xe9, 0x93, 0x94, 0xa7, 0xe5,
    0x84, 0x33, 0x8c, 0x18, 0xe7, 0xa5, 0xae, 0xcd,
    0x1d, 0x45, 0x77, 0xb4, 0x39, 0xfb, 0x0f, 0xd2,
    0x79, 0x03, 0x4a, 0xf0, 0x43, 0x1a, 0x25, 0xe7,
    0x07, 0xd6, 0x96, 0x4c, 0x42, 0xc8, 0xaf, 0xec,
    0xed, 0x9b, 0x72, 0x0e, 0x22, 0xa8, 0x96, 0xc8,
    0x76, 0xd9, 0xf1, 0x4f, 0xb2, 0x4c, 0x42, 0xae
};

int main(int argc, char** argv)
{
    // need to move this to a function
    range_decoder_state_t s;

    int read_head = 0;

    // initialize range decoder state
    s.rng = 128;
    uint8_t b0 = frame[read_head++];
    s.val = 127 - (b0 >> 1);
    s.saved_lsb = b0 & 0x01;
    print_range_decoder_state(&s);

    for (int i = 0; i < ARRAY_NUMEL(symbols_to_decode); i++) {
        // renormalize
        while (range_decoder_state_needs_renormalization(&s)) {
            range_decoder_state_renormalize(&s, frame[read_head++]);
            print_range_decoder_state(&s);
        }

        // decode
        range_decoder_state_decode_symbol(&s, symbols_to_decode[i]);
    }

    return 0;
}


int range_decoder_state_needs_renormalization(const range_decoder_state_t* s)
{
    return (s->rng <= (1 << 23));
}


void range_decoder_state_renormalize(range_decoder_state_t* s, uint8_t byte_in)
{
    printf("Renormalizing\n");

    assert(range_decoder_state_needs_renormalization(s));

    uint8_t newval = (s->saved_lsb << 7) | (byte_in >> 1);
    s->saved_lsb = byte_in & 0x01;

    s->rng <<= 8;
    s->val = ((s->val << 8) + (255 - newval)) & 0x7fffffff;
}


uint32_t range_decoder_state_decode_symbol(range_decoder_state_t* s,
                                           const symbol_context_t* sym)
{
    printf("Decoding symbol\n");
    print_symbol_context(sym);

    // figure out where in the range the symbol lies
    uint32_t symbol_location = (s->val / (s->rng / sym->ft)) + 1;
    uint32_t fs;
    if (symbol_location < sym->ft) {
        fs = sym->ft - symbol_location;
    } else {
        fs = 0;
    }

    // linear search over pdf values to find appropriate symbol
    bool symbol_found = false;
    uint32_t k = 0;
    for (int i = 0; i < sym->num_symbols; i++) {
        if ((sym->fl[i] <= fs) && (fs < sym->fh[i])) {
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
    s->val = s->val - (s->rng / sym->ft) * (sym->ft - sym->fh[k]);

    if (sym->fl[k] > 0) {
        s->rng = (s->rng / sym->ft) * (sym->fh[k] - sym->fl[k]);
    } else {
        s->rng = s->rng - (s->rng / sym->ft) * (sym->ft - sym->fh[k]);
    }

    printf("\n");

    return k;
}

void print_range_decoder_state(const range_decoder_state_t* s)
{
    printf("val: %08x    rng: %08x\n", s->val, s->rng);
}

void print_symbol_context(const symbol_context_t* sym)
{
    printf("Decoding symbol: %s\n", sym->symbol_context_name);
    printf("PDF: {");
    for (int i = 0; i < sym->num_symbols; i++) {
        printf("%i", sym->fh[i] - sym->fl[i]);
        if (i != (sym->num_symbols - 1)) printf(", ");
    }
    printf("} / %i\n", sym->ft);
}
