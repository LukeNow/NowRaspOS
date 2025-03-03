#include <stddef.h>
#include <stdint.h>

#include <kernel/sched.h>
#include <kernel/task.h>
#include <kernel/kalloc.h>
#include <kernel/printf.h>
#include <kernel/cpu.h>
#include <common/aarch64_common.h>
#include <common/assert.h>
#include <common/linkedlist.h>
#include <kernel/irq.h>
#include <common/lock.h>
#include <kernel/timer.h>

ll_head_t ready_list;
ll_head_t sleep_list;
ll_head_t wait_list;

//ll_head_t pause_list;
task_t idle_tasks[CORE_NUM + 1];
spinlock_t sched_lock;


task_t * test_task;

sleeplock_t test_lock;

void idle_loop()
{
    uint64_t reg;

    //irq_sync_el1(0x1111);

    //AARCH64_MRS(spsr_el1, reg);
    //DEBUG_DATA("reg=", reg);
    while (1) {

        //DEBUG_DATA("IDLE LOOP HIT CPU=", cpu_get_id());
        //DEBUG_DATA("Curr task=", cpu_get_currcpu_info()->curr_task);
        sched_schedule();
        aarch64_nop();
    }
}

void test_loop()
{
    while (1) {
        //DEBUG_DATA("TEST_LOOP=", cpu_get_id());

        //DEBUG_DATA("Curr task=", cpu_get_currcpu_info()->curr_task);

        lock_sleeplock(&test_lock);
        
        printf("ENTERED SLEEP LOCK\n");
        DEBUG_DATA("TEST_LOOP=", cpu_get_id());
        DEBUG_DATA("Curr task=", cpu_get_currcpu_info()->curr_task);



        lock_sleepunlock(&test_lock);

        //sched_add_ready_list(cpu_get_currcpu_info()->curr_task);
        aarch64_nop();

        sched_change_task_state(cpu_get_currcpu_info()->curr_task, TASK_READY);
        sched_schedule();
    }
}

/* SCHED LOCK HELD */
static void sched_add_list(ll_head_t * list, task_t * task)
{
    dll_node_t * node = &task->sched_node;

    if ((ll_head_t *)node->data == list) {
        DEBUG_PANIC("Task is already in the list");
    }

    if (node->data) {
        DEBUG_PANIC("Task might still be in another ready list.");
    }

    
    ll_push_list(list, node);
    
    node->data = (void*)list;
}

/* SCHED LOCK HELD */
static void sched_remove_list(ll_head_t * list, task_t * task)
{
    dll_node_t * node = &task->sched_node;

    if ((ll_head_t *)node->data != list) {
        DEBUG_PANIC("The current list is not the one we are removing from.");
    }

    if (!node->data) {
        DEBUG_PANIC("The current node is not in a list.");
    }

    ll_delete_node(list, &task->sched_node);

    node->data = NULL;
}

/* SCHED LOCK HELD */
void sched_change_task_state(task_t * task, task_state_t new_state)
{

    ll_head_t * curr_list;
    ll_head_t * new_list;

    curr_list = (ll_head_t *)task->sched_node.data;

    if (task->state == new_state) {
        DEBUG_PANIC("New task state change same as old");
    }

    if (curr_list) {
        sched_remove_list(curr_list, task);
    }

    switch (new_state) {
        case TASK_READY:
            new_list = &ready_list;
            break;
        case TASK_SLEEPING:
            new_list = &sleep_list;
            break;
        case TASK_WAITING:
            new_list = &wait_list;
            break;
        default:
            new_list = NULL;
        
    }

    if (new_list)
        sched_add_list(new_list, task);

    task->state = new_state;
}


void sched_sleep_on_lock(sleeplock_t * lock)
{
    //DEBUG("SLEEP ON LOCK ENTERED");
    uint64_t flags;
    task_t * curr_task;

    spinlock_irqsave(&sched_lock, &flags);
    lock_spinlock(&lock->list_lock);
    
    curr_task = cpu_get_currcpu_info()->curr_task;
    ll_push_list(&lock->sleep_list, &curr_task->lock_node);
    lock_spinunlock(&lock->list_lock);
    
   
    ASSERT_PANIC(curr_task->state == TASK_RUNNING, "The current task is not running");
    
    //DEBUG("ADDING TO SLEEP LIST");
    sched_change_task_state(curr_task, TASK_SLEEPING);
    
    spinunlock_irqrestore(&sched_lock, flags);

    //DEBUG("DESCHEDULING FROM SLEEP");
    sched_schedule();
}

void sched_wake_on_lock(sleeplock_t * lock)
{
    task_t * task;
    dll_node_t * node;
    uint64_t flags;

    spinlock_irqsave(&sched_lock, &flags);
   // DEBUG("WAKE ON LOCK ENTERED");
    lock_spinlock(&lock->list_lock);
    /* Wake 1 task on the lock */
    node = ll_pop_list(&lock->sleep_list);
    lock_spinunlock(&lock->list_lock);

    /* Empty list */
    if (!node)
        return;
    task = STRUCT_P(node, task_t, lock_node);
    

    //DEBUG_DATA("Waking task state=", task->state);
    ASSERT_PANIC(task->state == TASK_SLEEPING, "The current task is not sleeping");

    sched_change_task_state(task, TASK_READY);

    spinunlock_irqrestore(&sched_lock, flags);
}


void sched_timer_tasklet(ticks_t elapsed_time)
{
    task_t * task;
    dll_node_t * node;
    uint64_t flags;

    spinlock_irqsave(&sched_lock, &flags);

    DLL_ITER_LIST(&wait_list, node) {
        task = STRUCT_P(node, task_t, sched_node);
        if (task->time_used <= elapsed_time) {
            //sched_add_ready_list(task);
        } else {
            task->time_used -= elapsed_time;
        }
    }

    spinunlock_irqrestore(&sched_lock, flags);
}

/* SCHED LOCK HELD */
task_t * get_ready_task()
{
    task_t * ready_task;
    dll_node_t * node;

    if (!ll_list_size(&ready_list)) {
        ready_task = &idle_tasks[cpu_get_id()];
        //DEBUG("Idle task");
        return ready_task;
    }
    

    node = (dll_node_t *)ll_peek_first(&ready_list);


    //DEBUG_DATA("POPPED READY NODE=", node);
    if (!node) {
        DEBUG_PANIC("Pop ready task failed");
    }


    ready_task = STRUCT_P(node, task_t, sched_node);

    sched_change_task_state(ready_task, TASK_RUNNING);

    return ready_task;
}

void sched_pull_ready_task()
{
    task_t * next_task;
    uint64_t flags;
    
    spinlock_irqsave(&sched_lock, &flags);
    
    next_task = get_ready_task();

    //DEBUG_DATA("next_task=", next_task);
    //DEBUG_DATA("Next_task stack=", next_task->el1_stack_ptr);

    cpu_get_currcpu_info()->curr_task = next_task;
    
    spinunlock_irqrestore(&sched_lock, flags);
}

void sched_schedule()
{
    task_switch_sync();
}

void make_task(task_t * task, void * code_addr)
{
    uint64_t task_stack;
    task_stack = (uint64_t)kalloc_alloc(PAGE_SIZE, 0);
    ASSERT_PANIC(task_stack, "Could not alloc idle task stack");
    task_stack = (char *)task_stack + PAGE_SIZE;



    task_init(task, task_stack, code_addr);
}

void sched_init()
{
    cpu_info_t * cpu;
    task_t * task;
    cpu_init_info();

    ll_head_init(&ready_list, DLL_NODE_T);
    ll_head_init(&sleep_list, DLL_NODE_T);
    ll_head_init(&wait_list, DLL_NODE_T);

    for (int i = 0; i < CORE_NUM; i++) {
        DEBUG_DATA("PREPPING IDLE TASK AT=", &idle_tasks[i]);
        make_task(&idle_tasks[i], idle_loop);
        
        cpu = cpu_get_percpu_info(i);
        
        ASSERT_PANIC(cpu, "Cpu info is null");
        cpu->curr_task = &idle_tasks[i];
    }



    lock_init(&sched_lock);


    sched_test();
   // DEBUG_PANIC("STOP");
}

void sched_test()
{


    task_t * task;

    for (int i = 0; i < CORE_NUM; i++) {
        task = (task_t *)kalloc_alloc(sizeof(task_t), 0);
        //DEBUG_DATA("TASK INIT=", task);
        make_task(task, test_loop);
        sched_change_task_state(task, TASK_READY);
    }

    sleeplock_init(&test_lock);
}

void sched_start()
{   
    cpu_info_t * cpu;
    uint32_t id;

    id = cpu_get_id();
    cpu = cpu_get_currcpu_info();

    //DEBUG_DATA("Stack var address=", &cpu->curr_task->el1_stack_ptr);
    //DEBUG_DATA("Curr_task=", cpu->curr_task);
    //DEBUG_DATA("Curr_task stack=", cpu->curr_task->el1_stack_ptr);
    ASSERT_PANIC(cpu->curr_task == &idle_tasks[id], "Task we are starting is not the idle task");

    task_start();
}

