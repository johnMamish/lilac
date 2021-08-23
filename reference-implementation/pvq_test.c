#include <stdio.h>

#include "pvq.h"

int main()
{
    pvq_family_t s = {4, 2};
    printf("Calculating size of pvq family S(%i, %i):\n", s.l, s.k);
    printf("%10lu\n", pvq_family_calculate_size(&s));
    printf("\n");

    for (s.l = 2; s.l < 5; s.l++) {
        for(s.k = 1; s.k < 10; s.k++) {
            printf("Calculating size of pvq family S(%i, %i):\n", s.l, s.k);
            printf("%10lu\n", pvq_family_calculate_size(&s));
            printf("\n");
        }
    }
}
