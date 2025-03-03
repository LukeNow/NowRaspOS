#include <stdint.h>
#include <stddef.h>
#include <kernel/task.h>
#include <common/string.h>
#include <common/linkedlist.h>
#include <common/assert.h>

uint32_t top_task_id = 0;

//extern void task_init_stack(uint64_t * stack_addr, uint64_t * task_start, void * params);

uint32_t task_generate_id()
{
    uint32_t id = top_task_id;

    top_task_id ++;

    return id;
}

void task_init(task_t * task, uint64_t * stack_top, uint64_t * start_addr)
{   
    uint64_t * top;

    memset(task, 0, sizeof(size_t));

    top = task_init_stack(stack_top, start_addr, NULL);

    DEBUG_DATA("Task top=", top);
    task->el1_stack_ptr = top;
    task->task_id = task_generate_id();
    task->time_used = 0;
    task->state = TASK_NULL;
    DEBUG_DATA("task sched node=", &task->sched_node);

    // TODO correctly init nodes and lists
    ll_node_init(&task->sched_node, 0, DLL_NODE_T);
    ll_node_init(&task->lock_node, 0, LIST_NODE_T);
}