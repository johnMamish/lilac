/**
 * Test our own laplace decoder against that of the reference implementation.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "opus/celt/entdec.h"
#include "opus/celt/entenc.h"
#include "opus/celt/laplace.h"

#include "frame_context.h"
#include "symbol_context.h"

static const uint8_t e_prob_model_120s_intra[] =
{
    24, 179,  48, 138,  54, 135,  54, 132,  53, 134,  56, 133,  55, 132,
    55, 132,  61, 114,  70,  96,  74,  88,  75,  88,  87,  74,  89,  66,
    91,  67, 100,  59, 108,  50, 120,  40, 122,  37,  97,  43,  78,  50
};

uint8_t enc_buf[1024] = { 0 };

int main(int argc, char** argv)
{
    frame_context_t fctx = {.LM = 0, .C = 1, .intra = true};

    ec_enc enc;
    ec_enc_init(&enc, enc_buf, sizeof(enc_buf));



    for (int band = 0; band < 21; band++) {
        int fs0 = ((int)e_prob_model_120s_intra[2 * band]) << 7;
        int decay = ((int)e_prob_model_120s_intra[(2 * band) + 1]) << 6;

        for (int i = 0; i < 21; i++) {
            int val = -i;
            ec_laplace_encode(&enc, &val, fs0, decay);

            val = i;
            ec_laplace_encode(&enc, &val, fs0, decay);
        }

        symbol_context_t* sc = symbol_context_create_from_laplace(&fctx, band);
        print_symbol_context(sc);
        symbol_context_destroy(sc);
    }

    return 0;
}
