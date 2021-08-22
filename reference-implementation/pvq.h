#ifndef _PVQ_H
#define _PVQ_H

#include <stdint.h>


/**
 * if a vector's...
 *   * Length is L
 *   * Its L1 norm (called "number of pulses" in PVQ discussion) is K
 * then it's a member of pvq family S(L,K).
 *
 * e.g. X = <2, 0, -1, 1, 0> is a member of S(5, 4) because...
 *   * X's length is 5
 *   * 2 + 0 + 1 + 1 + 0 = 4
 */
typedef struct pvq_family {
    /// Number of elements in each vector of the pyramid
    int l;

    /// Number of pulses in each
    int k;
} pvq_family_t;


/**
 * Recursively calculates the number of vectors in a given PVQ family
 */
uint64_t pvq_family_calculate_size(const pvq_family_t* s);

/**
 *
 */


#endif
