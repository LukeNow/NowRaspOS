#ifndef __COMMON_H
#define __COMMON_H

#define CYCLE_WAIT(x) do { int i = (x); while (i--) { asm volatile ("nop"); } } while (0)
#define CYCLE_INFINITE do {  while(1){ asm volatile ("nop"); } } while (0)

typedef uint32_t flags_t;

#define BITS_PER_BYTE 8

#define KB(X) (X * 1024)
#define MB(X) (X * KB(1) * 1024)
#define GB(X) (X * MB(1) * 1024)

#define BITS(X) (UINT64_MAX >> (64 - X))

#define BIT_ALIGN(X, Y) ((X) & BITS(Y))
#define IS_ALIGNED(X, Y) ((X) % (Y) ? 0 : 1)
#define IS_BIT_ALIGNED(X,Y) (BIT_ALIGN(X, Y) ? 0 : 1)
#define ROUND_DOWN(x, size) ((x) - ((x) % (size)))
#define ROUND_UP(x, size) IS_ALIGNED((x), (size)) ? \
		(x) : ((x) + ((size) - ((x) % (size)))) 
#define ALIGN_DOWN(x, size) ROUND_DOWN((x) + (size) - 1, (size))
#define ALIGN_UP(x, size) ROUND_UP((x), (size))

#endif