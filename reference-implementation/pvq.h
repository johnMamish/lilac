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
    int32_t l;

    /// Number of pulses in each
    int32_t k;
} pvq_family_t;


typedef struct pvq_vector {
    int32_t  len;
    int32_t* x;
} pvq_vector_t;

/**
 * Calculates the number of vectors in a given PVQ family
 */
uint64_t pvq_family_calculate_size(const pvq_family_t* s);

/**
 * Given a number 'b' in the range {0, ..., N(L, K) - 1}, calculates the corresponding vector
 *
 */
pvq_vector_t* pvq_vector_create_from_codename(const pvq_family_t* s, uint64_t b);

void pvq_vector_destroy(pvq_vector_t*);

#endif
