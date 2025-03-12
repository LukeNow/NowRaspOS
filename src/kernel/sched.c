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
#include <common/queue.h>
#include <common/common.h>
#include <kernel/mbox.h>
#include <common/atomic.h>
#include <kernel/slock.h>

#define WAIT_QUEUE_NUM 59 // Hash friendly Queue num, Used by MACH kernel

queue_head_t ready_queue;

/* We want to have signals based on an ever increasing counter of EVENT_ID, 
 * and then have waiting threads hash to these waitqueues based on the id. 
 * We want to have a user-space friendly export for this Event_id num. */
queue_head_t wait_queue[WAIT_QUEUE_NUM];
spinlock_t wait_lock[WAIT_QUEUE_NUM];

#define wait_hash(event_id) \
        (uint32_t) (((event_id)) ? (event_id) % WAIT_QUEUE_NUM : NULL_EVENT_HASH)

#define IDLE_TASK (&idle_tasks[cpu_get_id()])

task_t idle_tasks[CORE_NUM + 1];
spinlock_t sched_lock;
uint64_t last_sched_timestamp;
event_id_t event_ids = 0;

/* TEST variables */
slock_t test_lock;
uint32_t array[32];

void idle_loop()
{
    while (1) {
        systemtimer_wait(1000);
        
        sched_yield();
    }
}

void test_loop2()
{
    while (1) {

        DEBUG_DATA("TEST_LOOP_2 = ", cpu_get_id());
        //sched_yield();
    }
}

void test_loop()
{
    uint32_t test = 0;
    uint32_t task_id = cpu_get_currcpu_info()->curr_task->task_id;
    
    array[task_id] = test;

    while (1) {
        //DEBUG_DATA("TEST_LOOP=", cpu_get_id());

        //DEBUG_DATA("Curr task=", cpu_get_currcpu_info()->curr_task);

        lock_slock(&test_lock);
        
        if (array[task_id] != test) {
            DEBUG_PANIC_ALL("NOT EQUAL");
        }
        DEBUG_DATA("Curr task=", cpu_get_currcpu_info()->curr_task->task_id);

        test++;
        array[task_id] = test;


        unlock_slock(&test_lock);
    }
}

uint64_t sched_enter_save()
{
    uint64_t flags;
    lock_spinlock_irqsave(&sched_lock, &flags);
    return flags;
}

void sched_exit_restore(uint64_t flags)
{
    unlock_spinlock_irqrestore(&sched_lock, flags);
}

void sched_enter()
{
    aarch64_dmb();
    irq_disable();
    lock_spinlock(&sched_lock);
}

void sched_exit()
{
    unlock_spinlock(&sched_lock);
    aarch64_dmb();
    irq_enable();
}

static task_t * get_idle_task()
{
    return &idle_tasks[cpu_get_id()];
}

static void sched_add_readyqueue(task_t * task)
{   
    if (!TASK_VALID(task)) {
        DEBUG_PANIC("TASK IS NOT VALID");
    }
    if (queue_valid(&task->sched_chain)) {
        DEBUG_PANIC("Task already in a queue");
    }
    enqueue_tail(&ready_queue, &task->sched_chain);
}

static task_t * sched_pop_readyqueue()
{   
    queue_entry_t qe;
    task_t * task;
    qe = dequeue_head(&ready_queue);
    if (!qe)
        return NULL;
    task = STRUCT_P(qe, task_t, sched_chain);
    queue_zero(qe);

    return task;
}

static void sched_rm_queue(task_t * task)
{   
    queue_entry_t qe = &task->sched_chain;
    if (queue_valid(qe)) {
        DEBUG_PANIC("Queue not valid");
    }
    rmqueue(qe);
    queue_zero(qe);
}

event_id_t event_init()
{
    return atomic_fetch_add_64(&event_ids, 1);
}

void event_waiton(event_id_t ev)
{
    uint64_t s = sched_enter_save();
    task_t * curr_task = CURR_TASK;
    uint32_t ev_hash = wait_hash(ev);
    queue_entry_t q;

    if (ev_hash == NULL_EVENT_HASH) {
        DEBUG_PANIC("NULL EVENT HASH");
        sched_exit_restore(s);
    }
    
    if (curr_task->wait_event || queue_valid(&curr_task->wait_chain)) {
        DEBUG_DATA("WAIT EVENT ID=", curr_task->wait_event);
        DEBUG_PANIC("Curr task is already waiting on an event");
        sched_exit_restore(s);
    }

    q = &wait_queue[ev_hash];

    lock_spinlock(&wait_lock[ev_hash]);

    curr_task->wait_event = ev;
    enqueue_tail(q, &curr_task->wait_chain);

    unlock_spinlock(&wait_lock[ev_hash]);

    sched_task_sleep();
}

void event_signal(event_id_t ev)
{
    task_t * task, * wake_task;
    queue_entry_t q;
    queue_entry_t qe;
    uint64_t s = sched_enter_save();
    uint32_t ev_hash = wait_hash(ev);

    if (ev_hash == NULL_EVENT_HASH) {
        sched_exit_restore(s);
    }

    lock_spinlock(&wait_lock[ev_hash]);
    q = &wait_queue[ev_hash];

    wake_task = NULL;
    queue_iter(q, qe) {
        task = qe_chain_access(qe, task_t, wait_chain);
        if (!TASK_VALID(task)) {
            DEBUG_PANIC("TASK IS NOT VALID");
        }

        if (task->wait_event == ev) {
            wake_task = task;
            break;
        }
    }

    if (wake_task) {
        rmqueue(&wake_task->wait_chain);
        /* Invalidate wait event */
        sched_task_wakeup(wake_task);
        wake_task->wait_event = 0;
        queue_zero(&wake_task->wait_chain);
    }

    unlock_spinlock(&wait_lock[ev_hash]);
    sched_exit_restore(s);
}

void event_signalall(event_id_t ev)
{
    task_t * task;
    queue_entry_t q;
    queue_entry_t qe;
    uint64_t s = sched_enter_save();
    uint32_t ev_hash = wait_hash(ev);

    if (ev_hash == NULL_EVENT_HASH) {
        sched_exit_restore(s);
    }

    lock_spinlock(&wait_lock[ev_hash]);
    q = &wait_queue[ev_hash];

    queue_iter(q, qe) {
        task = qe_chain_access(qe, task_t, wait_chain);
        
        if (task->wait_event == ev) {
            rmqueue(&task->wait_chain);
            sched_task_wakeup(task);
            /* Invalidate the wake event on the task*/
            task->wait_event = 0;
            queue_zero(&task->wait_chain);
        }
    }

    unlock_spinlock(&wait_lock[ev_hash]);
    sched_exit_restore(s);
}

/* ISR CONTEXT */
void sched_async_timeout()
{
    task_t * curr_task;
    uint64_t s = sched_enter_save();
    curr_task = CURR_TASK;

    if (curr_task->state & TASK_PAUSED && !(curr_task->state & TASK_IDLE)) {
        curr_task->state &= ~TASK_PAUSED;
        curr_task->state |= TASK_READY;
        if (!TASK_VALID(curr_task)) {
            DEBUG_PANIC_ALL("TASK NOT VALID");
        }

        sched_add_readyqueue(curr_task);
        aarch64_dmb();
        /* Does not return, SCHED LOCK held intentional. */
        task_switch_async();
    }

    /* If the current task is not paused, continue out of ISR normally
     * since we probably had another thread scheduled. */
    sched_exit_restore(s);
}

/* ISR Context - Called by localtimer IRQ
 * Core0 pauses the scheduler and bills time for all running threads.
 */
void sched_timer_isr()
{
    task_t * task;
    uint64_t elapsed;
    uint64_t s = sched_enter_save();

    elapsed = timer_difference(last_sched_timestamp, localtimer_gettime());
    last_sched_timestamp = localtimer_gettime();

    for (int i = 0; i < CORE_NUM; i++) {
        task = cpu_get_percpu_info(i)->curr_task;
        if (!TASK_VALID(task)) {
            DEBUG_PANIC_ALL("TASK IS NOT VALID");
        }
        /* Task is on the ready list or an idle task, ignore it. */
        if (task->state & TASK_READY || task->state & TASK_IDLE || task->state & TASK_PAUSED
            || task->state & TASK_UNINT) {
            continue;
        }

        /* First time billing this task? Ignore it because it could have been
         * just scheduled. */
        if (task->first_quanta) {
            task->first_quanta = 0;
            continue;
        }
        
        /* No time left, set the task as paused and interrupt the core its on
         * to run the timeout handler. */
        if (task->time_left <= elapsed) {
            task->state |= TASK_PAUSED;
            aarch64_dmb();
            /* Interrupt the core and make them execute the async_timeout handler */
            mbox_core_cmd_int(i, cpu_get_id(), CORE_EXEC, (uint32_t)sched_async_timeout);
        } else {
            task->time_left -= elapsed;
        }
    }

    aarch64_dmb();

    sched_exit_restore(s);
}

/* Select a next task to run. Called from task_switching context.
 * SCHED LOCK HELD */
task_t * sched_task_select()
{
    task_t * next_task;

    next_task = sched_pop_readyqueue();
    /* Ready queue is empty, get the idle task. */
    if (!next_task) {
        next_task = IDLE_TASK;
    }

    if (!TASK_VALID(next_task)) {
        DEBUG_DATA("TASK ADDR=", next_task);
        DEBUG_DATA("Failed task id=", next_task->task_id);
        DEBUG_DATA("Failed task state=", next_task->state);
        DEBUG_PANIC_ALL("Task is not valid");
    }
    
    next_task->state &= ~TASK_READY;

    return next_task;
}

/* Wakeup the task and put it on the ready queue. 
 * SCHED LOCK HELD */
void sched_task_wakeup(task_t * task)
{

    if (!TASK_VALID(task)) {
        DEBUG_PANIC("TASK NOT VALID");
    }

    if (!task->wait_event || !(task->state & TASK_WAITING)) {
        DEBUG_PANIC("No wait event or task is not in waiting state");
    }

    lock_spinlock(&task->lock);

    task->state &= ~TASK_WAITING;
    task->state |= TASK_READY;
    
    sched_add_readyqueue(task);

    unlock_spinlock(&task->lock);
}

/* Block the current task and stop it from being scheduled */
void sched_task_block()
{
    uint64_t s;
    task_t * curr_task;

    curr_task = CURR_TASK;

    lock_spinlock(&curr_task->lock);

    ASSERT_PANIC(curr_task->state & TASK_RUNNING, "Task is not running");

    curr_task->state &= ~TASK_RUNNING;
    curr_task->state |= TASK_BLOCKED;

    unlock_spinlock(&curr_task->lock);

    sched_schedule();
}

/* Put the current task to sleep, the current task is waiting on an event to be woken up from. */
void sched_task_sleep()
{
    task_t * curr_task;

    curr_task = CURR_TASK;

    if (!curr_task->wait_event) {
        DEBUG_PANIC("Curr task is not waiting for an event to be woken up from");
    }

    lock_spinlock(&curr_task->lock);
    curr_task->state |= TASK_WAITING;
    unlock_spinlock(&curr_task->lock);

    sched_task_block();
}

/*
 * Sync Sched order
 *
 * Initial sched call - (sched_yield, sched_task_block, sched_task_wakeup), aquire schedlock
 * SYNC_TASK_SAVE_CONTEXT - Registers pushed on stack and stack addr saved in curr_task
 * sched_task_select - Pull the next thread off the ready queue
 * Switch stack to new thread - the currtask on the cpu is now completly saved and can safelty
 *                              be resumed
 * sched_task_switch - Set the currtask ptr to the new task to switch to, set the state to running, 
 *                     reload any timer info, release sched lock
 * RESTORE_TASK_CONTEXT - Restore the stack again from saved ptr, restore the registers and jump to saved
 *                        code addr
 * */

/*
 * Async sched order
 * IRQ FIRED - Current stack and regs pushed onto stack, invoked by sched_timer_isr
 * sched_async_timeout - aquire sched lock, if the current task is paused push onto ready queue,
 *                       task_switch_async,
 * sched_task_select - Rest same as sync sched order
 * */

/* Switch to task
 * Called as the last function in the task switching
 * critial section.
 * 
 *
 *  SCHED_LOCK HELD
 *  CURR_TASK LOCK HELD
 */
void sched_task_switch(task_t * task)
{
    cpu_get_currcpu_info()->curr_task = task;
    if (!TASK_VALID(task)) {
        DEBUG_PANIC("TASK NOT VALID");
    }
    task->state &= ~TASK_BLOCK_STATES;
    task->state |= TASK_RUNNING;
    task_reload(task);

    aarch64_dmb();
    aarch64_isb();
    /* Sched exit to exit the scheduler and task switch critical section. */
    sched_exit();
}

void sched_schedule()
{
    task_switch_sync();
}

void sched_yield()
{   
    uint64_t flags;
    task_t * curr_task;
    
    sched_enter();

    curr_task = cpu_get_currcpu_info()->curr_task;

    if (!TASK_VALID(curr_task)) {
        DEBUG_PANIC_ALL("TASK INVALID");
    }

    if (curr_task != IDLE_TASK) {
        sched_add_readyqueue(curr_task);
        curr_task->state &= ~TASK_RUNNING;
        curr_task->state |= TASK_READY;
    }

    sched_schedule();
}

static void sched_test(void * code_addr)
{
    task_t * task;

    for (int i = 0; i < CORE_NUM; i++) {
        task = (task_t *)kalloc_alloc(sizeof(task_t), 0);
        task_create(task, code_addr);
        task->state = TASK_READY;
        sched_add_readyqueue(task);
    }
}

void sched_init()
{
    cpu_info_t * cpu;
    task_t * task;
    cpu_init_info();

    queue_init(&ready_queue);

    for (int i = 0; i < WAIT_QUEUE_NUM; i++) {

        queue_init(&wait_queue[i]);
        spinlock_init(&wait_lock[i]);
    }

    for (int i = 0; i < CORE_NUM; i++) {
        DEBUG("IDLE TASK");
        task_create(&idle_tasks[i], idle_loop);

        idle_tasks[i].state = TASK_IDLE;     
        cpu = cpu_get_percpu_info(i);
        
        ASSERT_PANIC(cpu, "Cpu info is null");
        cpu->curr_task = &idle_tasks[i];

    }

    lock_init(&sched_lock);
    last_sched_timestamp = localtimer_gettime();

    /* TEST INIT */
    sched_test(test_loop);
    sched_test(test_loop2);
    slock_init(&test_lock);
}

void sched_start()
{   
    cpu_info_t * cpu;
    uint32_t id;

    id = cpu_get_id();
    cpu = cpu_get_currcpu_info();

    ASSERT_PANIC(cpu->curr_task == &idle_tasks[id], "Task we are starting is not the idle task");

    task_start();
}

