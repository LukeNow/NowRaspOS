#ifndef __SCHED_H
#define __SCHED_H

#include <stdint.h>
#include <stddef.h>
#include <kernel/task.h>
#include <common/lock.h>

void sched_pull_ready_task();
void sched_init();
void sched_start();

void sched_wake_on_lock(sleeplock_t * lock);
void sched_sleep_on_lock(sleeplock_t * lock);

#endif