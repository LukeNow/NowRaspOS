#include <stddef.h>
#include <stdint.h>
#include <kernel/lock.h>
#include <kernel/atomic.h>
#include <common/common.h>

int lock_trylock(lock_t *lock)
{
    uint32_t ret = 0;
    
    ret = atomic_cmpxchg(lock, 0, 1);
    //0 for success in lock, 1 for lock is currently taken
    return ret; 
}

int lock_tryunlock(lock_t *lock)
{
    *lock = 0;
}

void lock_spinlock(lock_t *lock)
{
    while (lock_trylock(lock)) {
        CYCLE_WAIT(5);
    }
}

void lock_spinunlock(lock_t  *lock)
{
    *lock = 0;
}