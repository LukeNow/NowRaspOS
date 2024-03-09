#ifndef __KERN_DEFS_H
#define __KERN_DEFS_H

#include <stddef.h>

#define PAGE_SIZE 4096
#define PAGE_BIT_OFF 12
#define UPPER_ADDR_MASK 0xFFFF000000000000

#define KB(X) (X * 1024)
#define MB(X) (X * KB(1) * 1024)
#define GB(X) (X * MB(1) * 1024)

#define BITS(X) (UINT64_MAX >> (64 - X))

#define BIT_ALIGN(X, Y) ((X) & BITS(Y))
#define IS_ALIGNED(X, Y) ((X) % (Y) ? 0 : 1)
#define IS_BIT_ALIGNED(X,Y) (BIT_ALIGN(X, Y) ? 0 : 1)

#endif