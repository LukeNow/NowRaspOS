#ifndef __SLOCK_H
#define __SLOCK_H

#include <stddef.h>
#include <stdint.h>
#include <common/lock.h>
#include <kernel/sched.h>

typedef struct {
    lock_t lock;
    event_id_t ev_id;
} __attribute__((aligned(8))) slock_t;

void slock_init(slock_t * slock);
void lock_slock(slock_t * slock);
void unlock_slock(slock_t * slock);

#endif