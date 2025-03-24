#ifndef __TASK_H
#define __TASK_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <common/common.h>
#include <kernel/timer.h>
#include <common/linkedlist.h>
#include <kernel/timer.h>
#include <common/queue.h>
#include <common/lock.h>

#define TASK_NAME_LEN 32
#define TASK_MAGIC_VAL 0xdeadabcdbeeffeed
#define STACK_DEAD_VAL 0xdeadbeefabcdfeed

#define TASK_VALID(task_p) ((task_p) && (task_p)->magic == TASK_MAGIC_VAL)
/*
 * TASK_READY   - on ready queue and can be scheduled
 * TASK_RUNNING - currently running
 * TASK_WAITING - waiting on an event and is sleeping
 * TASK_PAUSED  - paused and is going to be descheduled or destroyed
 * TASK_IDLE    - idle task
 * TASK_UNINT   - uninterupptable, do not deschedule
 * Task_NULL    - last state descriptor, task state is invalid
 */
typedef enum task_state {
    TASK_READY = (1 << 0),
    TASK_RUNNING = (1 << 1),
    TASK_WAITING = (1 << 2),
    TASK_PAUSED = (1 << 3),
    TASK_IDLE = (1 << 4),
    TASK_UNINT = (1 << 5),
    TASK_BLOCKED = (1 << 6),
    TASK_NULL = (1 << 31)
} task_state_t;

#define TASK_BLOCK_STATES (TASK_WAITING | TASK_PAUSED | TASK_BLOCKED)

typedef uint64_t event_id_t;
#define NULL_EVENT_HASH ((uint32_t)~0)

typedef struct event {
    event_id_t id;
    time_us_t wait_time_left;
} event_t;

#define SCHED_WAIT_TICKS 32

typedef struct task {
    /* DO NOT MOVE, we grap the stack pointer from the top of struct. */
    uint64_t el1_stack_ptr;
    spinlock_t lock;
    /* queue chain structs for the ready queue and for a wait queue. */
    queue_chain_t sched_chain;
    queue_chain_t wait_chain;
    /* Magic val to check task struct integrity. */
    uint64_t magic;
    /* The time left of the given quanta, and its given amount of time to run. */
    time_us_t time_left;     // Time in us
    time_us_t quanta;        // Time in us
    /* Is this the first time we are billing the task? If it is we bill them next time 
     * since we dont want to bill tasks that have just been scheduled. */
    bool first_quanta; 
    /* If the task has been passed by the scheduler SCHED_WAIT_TICK times its priority will be elavated. */
    uint32_t sched_wait_ticks;
    uint32_t starting_prio;
    uint32_t task_id;
    event_t wait_event;
    task_state_t state;
    char name[TASK_NAME_LEN];
} task_t;

int task_lock(task_t * task);
void task_unlock(task_t * task);
void task_init(task_t * task, uint64_t * stack_top, uint64_t * start_addr);
void task_create(task_t * task, void * code_addr);
void task_reload(task_t * task);
extern uint64_t * task_init_stack(uint64_t * stack_addr, uint64_t * task_start, void * params);
extern void task_start();
extern void task_switch_async();
extern void task_switch_sync();


#endif