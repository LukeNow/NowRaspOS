#ifndef __BITS_H
#define __BITS_H

#include <stddef.h>
#include <stdint.h>

typedef uint8_t bit_t;
typedef uint64_t bits_t;

#define BITS_PER_BYTE 8

#define BITS(X) (UINT64_MAX >> (64 - X))
#define BIT_ALIGN(X, Y) ((X) & BITS(Y))
#define IS_BIT_ALIGNED(X,Y) (BIT_ALIGN(X, Y) ? 0 : 1)

unsigned int bits_count_64(uint64_t bits);
unsigned int bits_count_32(uint32_t bits);

#endif