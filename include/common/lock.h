#ifndef __LOCK_H
#define __LOCK_H

#include <stdint.h>
#include <stddef.h>

typedef uint64_t lock_t;
typedef lock_t spinlock_t;

#define DEFINE_LOCK(X) __attribute__((aligned(8))) static lock_t (X) = 0
#define DEFINE_SPINLOCK(X) __attribute__((aligned(8))) static spinlock_t (X) = 0

int lock_init(lock_t * lock);
int spinlock_init(spinlock_t * lock);
int sleeplock_init(sleeplock_t * lock);
int lock_trylock(lock_t * lock);
int lock_tryunlock(lock_t * lock);
void spinlock_irqsave(lock_t * lock, uint64_t * flags);
void spinunlock_irqrestore(lock_t * lock, uint64_t flags);


#endif