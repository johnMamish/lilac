#include <stdio.h>

#include "pvq.h"

int main()
{
    pvq_family_t s = {25, 25};
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

#if 1
    t = (pvq_family_t){25, 25};
    b = 3923;
    printf("Calculating vector for code b = %li in codebook S(%i, %i)\n", b, t.l, t.k);
    v = pvq_vector_create_from_codename(&t, b);
    for (int i = 0; i < t.l; i++) printf("% 5i,", v->x[i]);
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

    return 0;
}
