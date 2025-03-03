#include <stddef.h>
#include <stdint.h>
#include <kernel/irq.h>
#include <common/aarch64_common.h>
#include <common/lock.h>
#include <common/atomic.h>
#include <common/common.h>

int lock_init(lock_t * lock)
{
    *lock = 0;

    return 0;
}

int spinlock_init(spinlock_t * lock)
{
    *lock = 0;

    return 0;
}

int sleeplock_init(sleeplock_t * lock)
{
    spinlock_init(&lock->list_lock);
    ll_head_init(&lock->sleep_list, LIST_NODE_T);
    lock->lock = 0;

    return 0;
}

int lock_trylock(lock_t * lock)
{
    /* 0 for success in lock, 1 for lock is currently taken or there is contention. */
    return atomic_cmpxchg_64(lock, 0, 1);
}

int lock_tryunlock(lock_t * lock)
{
    aarch64_dmb();
    *lock = 0;
}

void lock_spinlock(lock_t * lock)
{
    while (lock_trylock(lock)) {
        CYCLE_WAIT(5);
    }
}

void spinlock_irqsave(lock_t * lock, uint64_t * flags)
{
    int ret;

    irq_save_disable(flags);
    lock_spinlock(lock) ;  
}

void spinunlock_irqrestore(lock_t * lock, uint64_t flags)
{
    lock_tryunlock(lock);
    irq_restore(flags);
}

void lock_spinunlock(lock_t  * lock)
{
    aarch64_dmb();
    *lock = 0;
}