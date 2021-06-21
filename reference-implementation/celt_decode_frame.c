/**
 * Decodes a single CELT frame and dumps human-readable information about it to stdout.
 */

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "range_coder.h"
#include "symbol_context.h"

#define ARRAY_NUMEL(arr) (sizeof(arr) / (sizeof(arr[0])))


const symbol_context_t* symbols_to_decode[] = {
    &CELT_silence_context,
    &CELT_post_filter_context,
    &CELT_transient_context,
    &CELT_intra_context,
};

uint8_t frame[] =
{
    0xf0, 0x5f, 0x48, 0x95, 0x5a, 0x27, 0x38, 0x72,
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

void coarse_energy_decode(const frame_context_t* context, range_decoder_t* rd, float* out);

int main(int argc, char** argv)
{
    range_decoder_t* range_decoder = range_decoder_create(frame + 1, sizeof(frame) - 1);

    // Initialize frame context info.
    frame_context_t context;
    uint8_t toc = frame[0];
    context.LM = (toc >> 3) & 0x03;
    context.C = 1;

    // Decode 'static' symbols for frame context
    context.silence = range_decoder_decode_symbol(range_decoder, &CELT_silence_context); printf("\n");
    context.post_filter = range_decoder_decode_symbol(range_decoder, &CELT_post_filter_context); printf("\n");
    if (context.post_filter) {
        printf("post-filter specified in frame but not implemented!\n");
        return -1;
    }
    context.transient = range_decoder_decode_symbol(range_decoder, &CELT_transient_context); printf("\n");
    context.intra = range_decoder_decode_symbol(range_decoder, &CELT_intra_context); printf("\n");

    // Decode coarse energy


    range_decoder_destroy(range_decoder);

    return 0;
}

/**
 * @param[in]     context    Struct containing basic frame info like frame size & number of channels
 * @param[in,out] rd         Range decoder which is advanced to the point where it's about to read
 *                           coarse energy.
 * @param[out]    out        Caller should point this to an array containing 21 elements. Returns
 *                           the coarse energy symbols.
 */
void coarse_energy_decode(const frame_context_t* context, range_decoder_t* rd, float* out)
{
    for (int j = 0; j < 21; j++) {
        symbol_context_t* coarse_symbol_j = symbol_context_create_from_laplace(context, j);

        out[j] = range_decoder_decode_symbol(range_decoder, coarse_symbol_j);
        printf("\n");

        symbol_context_destroy(coarse_symbol);
    }
}
