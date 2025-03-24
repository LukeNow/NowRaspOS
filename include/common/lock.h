#ifndef __LOCK_H
#define __LOCK_H

#include <stdint.h>
#include <stddef.h>
#include <common/linkedlist.h>

typedef uint64_t lock_t;
typedef lock_t spinlock_t;

#define DEFINE_LOCK(X) __attribute__((aligned(8))) static lock_t (X) = 0
#define DEFINE_SPINLOCK(X) __attribute__((aligned(8))) static spinlock_t (X) = 0

typedef struct {
    uint64_t max_num;
    uint64_t num;
} semaphore_t  __attribute__ ((aligned (8)));

void init_sempahore(semaphore_t * sem, unsigned int num);
void semaphore_take(semaphore_t * sem);
void semaphore_give(semaphore_t * sem);
void semaphore_set(semaphore_t * sem, unsigned int expected, unsigned int new_val);

typedef struct {
    uint64_t writer_waiting;
    uint32_t max_reader_count;
    semaphore_t sem;
} rw_lock_t  __attribute__ ((aligned (8)));

int lock_init(lock_t * lock);
int spinlock_init(spinlock_t * lock);
int lock_trylock(lock_t * lock);
int unlock_trylock(lock_t * lock);
void lock_spinlock_irqsave(lock_t * lock, uint64_t * flags);
void unlock_spinlock_irqrestore(lock_t * lock, uint64_t flags);
void lock_spinlock(lock_t * lock);
void unlock_spinlock(lock_t * lock);

void rwlock_init(rw_lock_t * lock, unsigned int max_reader_count);
void rwlock_read_lock(rw_lock_t * lock);
void rwlock_write_lock(rw_lock_t * lock);
void rwlock_read_unlock(rw_lock_t * lock);
void rwlock_write_unlock(rw_lock_t * lock);

#endif