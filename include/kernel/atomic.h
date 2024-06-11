#ifndef __ATOMIC_H
#define __ATOMIC_H


#define ATOMIC_UINT64(X) __attribute__((aligned(8))) static uint64_t (X) = 0
#define ATOMIC_UINT32(X) __attribute__((aligned(4))) static uint32_t (X) = 0

int atomic_cmpxchg(uint64_t *ptr, uint64_t old, uint64_t new);
int atomic_fetch_add(uint64_t *addr, uint64_t val);
int atomic_fetch_sub(uint64_t *addr, uint64_t val);
int atomic_fetch_or(uint64_t *addr, uint64_t val);



#endif