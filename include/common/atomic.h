#ifndef __ATOMIC_H
#define __ATOMIC_H

#include <stdint.h>
#include <stddef.h>

#define ATOMIC_UINT64(X) __attribute__((aligned(8))) static uint64_t (X) = 0
#define ATOMIC_UINT32(X) __attribute__((aligned(4))) static uint32_t (X) = 0

typedef uint32_t atomic32_t;
typedef uint64_t atomic64_t;

extern int atomic_semaphore_give(uint64_t * count);
extern int atomic_semaphore_take(uint64_t * count);
extern int atomic_cmpxchg_64(uint64_t *ptr, uint64_t old_val, uint64_t new_val);
/* Try the cmpxchg a number of times. */
int atomic_cmpxchg_try_64(uint64_t * ptr, uint64_t old_val, uint64_t new_val, uint32_t tries);
/* Eventually for these atomic ops we can eventually do it how linux does it and
 * do some pattern substitution with the ops since most of the assembly is the same. */
extern uint64_t atomic_fetch_add_64(uint64_t *addr, uint64_t val);
extern uint64_t atomic_fetch_sub_64(uint64_t *addr, uint64_t val);
extern uint64_t atomic_fetch_or_64(uint64_t *addr, uint64_t val);
extern uint32_t atomic_ld_32(uint32_t * ptr);
extern uint64_t atomic_ld_64(uint64_t * ptr);
extern int atomic_str_32(uint32_t * ptr, uint32_t val);
extern int atomic_str_64(uint64_t * ptr, uint64_t val);

#endif