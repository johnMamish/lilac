#include <stdio.h>
#include <stdlib.h>
#include "pvq.h"

/**
 * pvq_lookup[L][K] gives the size of the codebook for those values of L and K for L and K < 16.
 */

static uint64_t pvq_lookup[16][16] = {
    { 0 },
    {  1,            2,            2,            2,            2,            2,            2,            2,            2,            2,            2,            2,            2,            2,            2,            2},
    {  1,            4,            8,           12,           16,           20,           24,           28,           32,           36,           40,           44,           48,           52,           56,           60},
    {  1,            6,           18,           38,           66,          102,          146,          198,          258,          326,          402,          486,          578,          678,          786,          902},
    {  1,            8,           32,           88,          192,          360,          608,          952,         1408,         1992,         2720,         3608,         4672,         5928,         7392,         9080},
    {  1,           10,           50,          170,          450,         1002,         1970,         3530,         5890,         9290,        14002,        20330,        28610,        39210,        52530,        69002},
    {  1,           12,           72,          292,          912,         2364,         5336,        10836,        20256,        35436,        58728,        93060,       142000,       209820,       301560,       423092},
    {  1,           14,           98,          462,         1666,         4942,        12642,        28814,        59906,       115598,       209762,       361550,       596610,       948430,      1459810,      2184462},
    {  1,           16,          128,          688,         2816,         9424,        27008,        68464,       157184,       332688,       658048,      1229360,      2187520,      3732560,      6140800,      9785072},
    {  1,           18,          162,          978,         4482,        16722,        53154,       148626,       374274,       864146,      1854882,      3742290,      7159170,     13079250,     22952610,     38878482},
    {  1,           20,          200,         1340,         6800,        28004,        97880,       299660,       822560,      2060980,      4780008,     10377180,     21278640,     41517060,     77548920,    139380012},
    {  1,           22,          242,         1782,         9922,        44726,       170610,       568150,      1690370,      4573910,     11414898,     26572086,     58227906,    121023606,    240089586,    457018518},
    {  1,           24,          288,         2312,        14016,        68664,       284000,      1022760,      3281280,      9545560,     25534368,     63521352,    148321344,    327572856,    688686048,   1385794152},
    {  1,           26,          338,         2938,        19266,       101946,       454610,      1761370,      6065410,     18892250,     53972178,    143027898,    354870594,    830764794,   1847023698,   3921503898},
    {  1,           28,          392,         3668,        25872,       147084,       703640,      2919620,     10746400,     35704060,    108568488,    305568564,    803467056,   1989102444,   4666890936,  10435418532},
    {  1,           30,          450,         4510,        34050,       207006,      1057730,      4680990,     18347010,     64797470,    209070018,    623207070,   1732242690,   4524812190,  11180805570,  26283115038},
};

uint64_t pvq_family_calculate_size(const pvq_family_t* s)
{
    uint64_t sum = 0;

    // base case
    if (s->l == 1) {
        if (s->k > 0) {
            return 2;
        } else {
            return 1;
        }
    } else if ((s->l < 16) && (s->k < 16)) {
        return pvq_lookup[s->l][s->k];
    } else {
        pvq_family_t subfamily;

        // for k > 1, we add 1 subfamily size for x_0 = +k and x_0 = -k.
        subfamily.l = s->l - 1;
        for (subfamily.k = 0; subfamily.k < s->k; subfamily.k++) {
            sum += 2 * pvq_family_calculate_size(&subfamily);
        }

        // for x_0 = 0, we only add the size of the subfamily * 1 because +0 = -0
        subfamily.k = s->k;
        sum += pvq_family_calculate_size(&subfamily);
    }

    return sum;
}

/**
 * recursive helper function for pvq_vector_create_from_codename
 */
static void pvq_vector_create_from_codename_r(const pvq_family_t* s, uint64_t b, int32_t* x)
{
    // base cases
    if (s->l == 0) {
        return;
    } else if (s->l == 1) {
        x[0] = (b < s->k) ? s->k : -s->k;
        return;
    }

    uint64_t upper_b, lower_b;

    // special case: x_i = 0
    pvq_family_t proposed_subfamily = {s->l - 1, s->k};
    lower_b = 0;
    upper_b = pvq_family_calculate_size(&proposed_subfamily);
    if (b < upper_b) {
        x[0] = 0;
        pvq_vector_create_from_codename_r(&proposed_subfamily, b - lower_b, &x[1]);
        return;
    }

    for (int proposed_k = 1; proposed_k <= s->k; proposed_k++) {
        pvq_family_t proposed_subfamily = {s->l - 1, s->k - proposed_k};
        uint64_t subfamily_size = pvq_family_calculate_size(&proposed_subfamily);

        for (int sgn = 1; sgn >= -1; sgn -= 2) {
            lower_b = upper_b;
            upper_b += subfamily_size;

            if (b < upper_b) {
                x[0] = proposed_k * sgn;
                pvq_vector_create_from_codename_r(&proposed_subfamily, b - lower_b, &x[1]);
                return;
            }
        }
    }

    // error!
    printf("[pvq_vector_create_from_codename_r] search failed while proposed_k in bounds\n");
}

pvq_vector_t* pvq_vector_create_from_codename(const pvq_family_t* s, uint64_t b)
{
    pvq_vector_t* result = calloc(1, sizeof(pvq_vector_t));
    result->len = s->l;
    result->x = calloc(result->len, sizeof(int32_t));

    pvq_vector_create_from_codename_r(s, b, result->x);

    return result;
}
