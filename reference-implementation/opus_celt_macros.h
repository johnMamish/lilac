/**
 * All material in this file is taken from the official opus reference implementation.
 */

#ifndef _OPUS_CELT_MACROS_H
#define _OPUS_CELT_MACROS_H

#include <stdint.h>

/* Multiplies two 16-bit fractional values. Bit-exactness of this macro is important */
#define FRAC_MUL16(a,b) ((16384+((int32_t)(int16_t)(a)*(int16_t)(b)))>>15)

/*  */
static int ec_ilog(int32_t _v) __attribute__((unused));
static int ec_ilog(int32_t _v){
    int ret;
    int m;
    ret=!!_v;
    m=!!(_v&0xFFFF0000)<<4;
    _v>>=m;
    ret|=m;
    m=!!(_v&0xFF00)<<3;
    _v>>=m;
    ret|=m;
    m=!!(_v&0xF0)<<2;
    _v>>=m;
    ret|=m;
    m=!!(_v&0xC)<<1;
    _v>>=m;
    ret|=m;
    ret+=!!(_v&0x2);
    return ret;
}

# define EC_ILOG(_x) (ec_ilog(_x))

#endif
