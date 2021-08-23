#include "pvq.h"

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

//uint64_t pvq_
