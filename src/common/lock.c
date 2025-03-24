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

void init_sempahore(semaphore_t * sem, unsigned int max_num)
{
    sem->max_num = max_num;
    sem->num = max_num;
}

void semaphore_take(semaphore_t * sem)
{
    atomic_semaphore_take(&sem->num);
}

void semaphore_give(semaphore_t * sem)
{
    atomic_semaphore_give(&sem->num);
}

void semaphore_set(semaphore_t * sem, unsigned int expected, unsigned int new_val)
{
    int ret;
    do {
        ret = atomic_cmpxchg_64(&sem->num, expected, new_val);
    } while (ret);
}

/* Max_reader_count can be set to MAX_INT if you want unlimited reads. */
void rwlock_init(rw_lock_t * lock, unsigned int max_reader_count)
{
    lock->max_reader_count = max_reader_count;
    lock->writer_waiting = 0;
    init_sempahore(&lock->sem, max_reader_count);
}

void rwlock_read_lock(rw_lock_t * lock)
{
    while (atomic_ld_64(&lock->writer_waiting)) {}
    semaphore_take(&lock->sem);
}

void rwlock_write_lock(rw_lock_t * lock)
{
    atomic_fetch_add_64(&lock->writer_waiting, 1);
    semaphore_set(&lock->sem, lock->max_reader_count, 0);
}

void rwlock_read_unlock(rw_lock_t * lock)
{
    semaphore_give(&lock->sem);
    aarch64_dmb();
}

void rwlock_write_unlock(rw_lock_t * lock)
{
    semaphore_set(&lock->sem, 0, lock->max_reader_count);
    atomic_fetch_sub_64(&lock->writer_waiting, 1);
    aarch64_dmb();
}
