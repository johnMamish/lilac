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

// V(N, K) = V(N-1,K) + V(N,K-1) + V(N-1,K-1), with V(N,0) = 1 and V(0,K) = 0
uint64_t pvq_family_calculate_size2(const pvq_family_t* s)
{
    if (s->l == 1) {
        return (s->k > 0) ? 2 : 1;
    } else if (s->k == 1) {
        return 2 * s->l;
    } else {
        uint64_t sum = 0;

        pvq_family_t subfamily = {(s->l - 1), s->k};
        sum += pvq_family_calculate_size2(&subfamily);

        subfamily = (pvq_family_t){s->l, s->k - 1};
        sum += pvq_family_calculate_size2(&subfamily);

        subfamily = (pvq_family_t){(s->l - 1), (s->k - 1)};
        sum += pvq_family_calculate_size2(&subfamily);

        return sum;
    }
}


//uint64_t pvq_
