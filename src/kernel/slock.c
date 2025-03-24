#include <stddef.h>
#include <stdint.h>
#include <common/lock.h>
#include <common/string.h>
#include <kernel/slock.h>
#include <kernel/sched.h>
#include <common/assert.h>

#define SLOCK_TRIES 20
#define WAIT_TIME_MS 100

void slock_init(slock_t * slock)
{
    memset(slock, 0, sizeof(slock_t));
    lock_init(&slock->lock);
    slock->ev_id = event_init();
    DEBUG_DATA("SLOCK wait id =", slock->ev_id);
}

void lock_slock(slock_t * slock)
{
    unsigned int tries = 0;
    int ret = lock_trylock(&slock->lock);

    while (ret) {

        if (tries > SLOCK_TRIES) {
            tries = 0;
            event_waiton(slock->ev_id, TIME_MS_TO_US(WAIT_TIME_MS));
        }

        ret = lock_trylock(&slock->lock);
        tries ++;
    }
}

void unlock_slock(slock_t * slock)
{
    unlock_trylock(&slock->lock);

    event_signal(slock->ev_id);
}