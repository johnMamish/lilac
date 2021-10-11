#include <math.h>
#include <stdio.h>

#include "pvq.h"

/**
 *
 */
static int get_pulses(int i)
{
    if (i < 8) {
        return i;
    } else {
        return (8 + (i % 8)) << ((i / 8) - 1);
    }
}

int main()
{
    pvq_family_t s = {5, 30};
#if 1
    printf("Calculating size of pvq family S(%i, %i):\n", s.l, s.k);
    printf("%10lu\n", pvq_family_calculate_size(&s));
    printf("\n");
    #endif

    for (s.l = 2; s.l < 5; s.l++) {
        for(s.k = 1; s.k < 10; s.k++) {
            printf("Calculating size of pvq family S(%i, %i):\n", s.l, s.k);
            printf("%10lu\n", pvq_family_calculate_size(&s));
            printf("\n");
        }
    }

    pvq_family_t t = {4, 2};
    uint64_t b = 9;
    printf("Calculating vector for code b = %li in codebook S(%i, %i)\n", b, t.l, t.k);
    pvq_vector_t* v = pvq_vector_create_from_codename(&t, b);
    for (int i = 0; i < t.l; i++) printf("% 5i,", v->x[i]);
    printf("\n\n");

#if 0
    t = (pvq_family_t){25, 25};
    b = 3923;
    printf("Calculating vector for code b = %li in codebook S(%i, %i)\n", b, t.l, t.k);
    v = pvq_vector_create_from_codename(&t, b);
    for (int i = 0; i < t.l; i++) printf("% 5i,", v->x[i]);
    printf("\n\n");
#endif

#if 1
    pvq_family_t r = {3, 2};
    printf("Listing all vectors in family S(%i, %i)\n", r.l, r.k);
    printf("------------------------------------------------\n");
    for (int i = 0; i < pvq_family_calculate_size(&r); i++) {
        pvq_vector_t* v = pvq_vector_create_from_codename(&r, i);
        printf("%3i: ", i);
        for (int i = 0; i < r.l; i++) printf("%3i,", v->x[i]);
        printf("\n");
        pvq_vector_destroy(v);
    }
    printf("\n\n");
#endif

#if 0
    pvq_family_t u;
    printf("table for L = [1, 16), K = [0, 16)\n"
           "------------------------------------------------\n");
    for (u.l = 1; u.l < 16; u.l++) {
        for (u.k = 0; u.k < 16; u.k++) {
            printf("%12li, ", pvq_family_calculate_size(&u));
        }
        printf("\n");
    }
#endif

    pvq_family_t u;
    printf("bit cache"
           "------------------------------------------------\n");

    //static int lvals[] = {1, 2, 3, 4, 6, 8, 9, 11, 12, 16, 18, 22, 24, 32, 36, 44, 48, 64, 72, 88, 96, 144, 176};
    static int lvals[] = {1, 2, 3, 4, 6, 9, 11, 8, 12, 18, 22, 16, 24, 36, 44, 32, 48, 72, 88, 64, 96, 144, 176};
    for (int i = 0; i < (sizeof(lvals) / sizeof(lvals[0])); i++) {
        u.l = lvals[i];
        float bits;
        int idx = 0;
        //printf("L = %3i:", u.l);
        while (idx < 41) {
            u.k = get_pulses(idx);
            bits = 8 * log2(pvq_family_calculate_size(&u));
            int bitsi = (int)floor(bits - 1e-6);
            if (bitsi > 255) {
                break;
            } else {
                //printf("%6.1f,", bits);
                printf("%5i,", (int)floor(bits - 1e-9));
            }

            idx++;
        }
        printf("\n");
    }

    return 0;
}
