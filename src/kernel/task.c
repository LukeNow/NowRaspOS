#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <common/common.h>
#include <kernel/task.h>
#include <kernel/mmu.h>
#include <kernel/kalloc.h>
#include <common/string.h>
#include <common/linkedlist.h>
#include <common/assert.h>
#include <common/lock.h>

uint32_t top_task_id = 0;

#define TASK_QUANTA_MS 1000
#define TASK_QUANTA_US (TIME_MS_TO_US(TASK_QUANTA_MS)) // 10MS, 10000 us

int task_lock(task_t * task)
{
    return lock_trylock(&task->lock);
}

void task_unlock(task_t * task)
{
    unlock_trylock(&task->lock);
}

uint32_t task_generate_id()
{
    uint32_t id = top_task_id;

    top_task_id ++;

    return id;
}

void task_reload(task_t * task)
{
    task->first_quanta = true;
    task->time_left = task->quanta;
    task->sched_wait_ticks = SCHED_WAIT_TICKS;
}

void task_init(task_t * task, uint64_t * stack_top, uint64_t * start_addr)
{   
    uint64_t * top;

    memset(task, 0, sizeof(size_t));

    top = task_init_stack(stack_top, start_addr, NULL);

    task->el1_stack_ptr = top;
    task->task_id = task_generate_id();
    task->quanta = TASK_QUANTA_US;
    task_reload(task);

    task->magic = TASK_MAGIC_VAL;
    queue_zero(&task->sched_chain);
    queue_zero(&task->wait_chain);
    task->wait_event.id = 0;

    spinlock_init(&task->lock);
    
    DEBUG_DATA("Task INIT id = ", task->task_id);
    DEBUG_DATA("TASK INIT Task =", task);
    DEBUG_DATA("Tasl INIT Task end addr = ", (uint8_t*)task + sizeof(task_t));
    DEBUG_DATA("Task stack top =", top);

}

void task_create(task_t * task, void * code_addr)
{
    uint64_t task_stack;
    
    task_stack = (uint64_t)kalloc_alloc(PAGE_SIZE, 0);
    memset_64((uint64_t *)task_stack, STACK_DEAD_VAL, PAGE_SIZE);
    ASSERT_PANIC(task_stack, "Could not alloc idle task stack");
    task_stack = (char *)task_stack + (PAGE_SIZE);

    task_init(task, task_stack, code_addr);
}