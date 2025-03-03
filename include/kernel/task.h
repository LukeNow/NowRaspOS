#ifndef __TASK_H
#define __TASK_H

#include <stdint.h>
#include <stddef.h>

#include <kernel/timer.h>
#include <common/linkedlist.h>
#include <kernel/timer.h>

#define TASK_NAME_LEN 32


typedef enum task_state {
    TASK_READY,
    TASK_RUNNING,
    TASK_SLEEPING,
    TASK_WAITING,
    TASK_PAUSED,
    TASK_IDLE,
    TASK_NULL
} task_state_t;

typedef struct task {
    /* DO NOT MOVE, we grap the stack pointer from the top of struct. */
    uint64_t el1_stack_ptr;
    /* If we are in a wait context this time is the time remaining */
    ticks_t time_used;
    uint32_t task_id;
    /* For sched ready lists etc, we want adds/removes to be fast so make it a doubly
     * linked list. Data points to the list its in. */
    dll_node_t sched_node;
    /* For per lock waiting lists. Lock lists shouldnt be long so making it a singly linked list
     * should be okay. */
    list_node_t lock_node;
    task_state_t state;
    char name[TASK_NAME_LEN];
} task_t;

void task_init(task_t * task, uint64_t * stack_top, uint64_t * start_addr);
extern uint64_t * task_init_stack(uint64_t * stack_addr, uint64_t * task_start, void * params);
extern void task_start();
extern void task_start();
extern void task_switch_async();
extern void task_switch_sync();


#endif