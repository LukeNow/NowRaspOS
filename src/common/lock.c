#include <stddef.h>
#include <stdint.h>
#include <kernel/irq.h>
#include <common/aarch64_common.h>
#include <common/lock.h>
#include <common/atomic.h>
#include <common/common.h>
#include <kernel/task.h>
#include <kernel/cpu.h>
#include <kernel/sched.h>
#include <common/queue.h>

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

int lock_trylock(lock_t * lock)
{
    /* 0 for success in lock, 1 for lock is currently taken or there is contention. */
    return atomic_cmpxchg_64(lock, 0, 1);
}

int unlock_trylock(lock_t * lock)
{
    aarch64_dmb();
    *lock = 0;

    return 0;
}

void lock_spinlock(lock_t * lock)
{
    while (lock_trylock(lock)) {
        CYCLE_WAIT(5);
    }
}

void lock_spinlock_irqsave(lock_t * lock, uint64_t * flags)
{
    irq_save_disable(flags);
    lock_spinlock(lock) ;  
}

void unlock_spinlock_irqrestore(lock_t * lock, uint64_t flags)
{
    unlock_spinlock(lock);
    irq_restore(flags);
}

void unlock_spinlock(lock_t  * lock)
{
    aarch64_dmb();
    *lock = 0;
}