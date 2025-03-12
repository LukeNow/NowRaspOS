#ifndef __SCHED_H
#define __SCHED_H

#include <stdint.h>
#include <stddef.h>
#include <kernel/task.h>
#include <common/lock.h>

/* Event definitions */
/* Returns an event id that tasks can wait on */
event_id_t event_init();
/* Wait on an event id. Sleeps the current task. */
void event_waiton(event_id_t ev);
/* Signal that an event has happened and wake one task. Wakes 1 task on the event wait list. */
void event_signal(event_id_t ev);
/* Signal that an event has happened and wake all waiting tasks. */
void event_signalall(event_id_t ev);

/* Task primitives. MUST HOLD SCHED LOCK.*/
task_t * sched_task_select();
void sched_task_wakeup(task_t * task);
void sched_task_block();
void sched_task_sleep();

void sched_init();
void sched_start();
void sched_yield();
void sched_schedule();


void sched_timer_isr();
void sched_async_timeout();

#endif