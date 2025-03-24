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
#include <kernel/klog.h>

#define WAIT_QUEUE_NUM 59 // Hash friendly Queue num, Used by MACH kernel
#define READY_QUEUE_NUM 8
#define READY_QUEUE_LAST (READY_QUEUE_NUM - 1)
#define READY_QUEUE_FIRST 0

queue_head_t ready_queue[READY_QUEUE_NUM];
spinlock_t ready_lock; // One lock for all the queues

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
time_us_t last_sched_timestamp;
event_id_t event_ids = 0;

uint32_t sched_ready = 0;
/* TEST variables */
slock_t test_lock;
uint32_t array[32];

void idle_loop()
{
    while (1) {
        //systemtimer_wait(1000);
        
        sched_yield();
    }
}

void test_loop2()
{

    uint32_t task_id = cpu_get_currcpu_info()->curr_task->task_id;
    while (1) {
        
        
        klog_printf("Curr task id= %d\n", task_id);
        sched_yield();
    }
}

void test_loop()
{
    uint32_t test = 0;
    uint32_t task_id = cpu_get_currcpu_info()->curr_task->task_id;
    
    array[task_id] = test;

    while (1) {

        lock_slock(&test_lock);
        
        klog_printf("Curr task id= %d\n", task_id);

        test++;

        unlock_slock(&test_lock);

        sched_yield();
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

static void sched_add_readyqueue(task_t * task, unsigned int ready_queue_num)
{   
    if (!TASK_VALID(task) && queue_valid(&task->sched_chain) && ready_queue_num < READY_QUEUE_NUM) {
        DEBUG_PANIC("TASK IS NOT VALID");
    }

    enqueue_tail(&ready_queue[ready_queue_num], &task->sched_chain);
}

static task_t * sched_pop_readyqueue()
{   
    queue_entry_t qe;
    queue_head_t * rq;
    task_t * task = NULL;
    
    for (int i = 0; i < READY_QUEUE_NUM; i++) {
        rq = &ready_queue[i];
        qe = dequeue_head(rq);
        if (!qe)
            continue;
        task = qe_chain_access(qe, task_t, sched_chain);
        queue_zero(qe);
        break;
    }

    
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

/* Just incremement the event ID as the queue position is just a hash. */
event_id_t event_init()
{
    return atomic_fetch_add_64(&event_ids, 1);
}

/* Zero wait_time_us means do not time the wait. */
void event_waiton(event_id_t ev, time_us_t wait_time)
{
    uint64_t s = sched_enter_save();
    task_t * curr_task = CURR_TASK;
    uint32_t ev_hash = wait_hash(ev);
    queue_entry_t q;

    if (ev_hash == NULL_EVENT_HASH) {
        DEBUG_PANIC("NULL EVENT HASH");
        sched_exit_restore(s);
    }
    
    if (curr_task->wait_event.id || queue_valid(&curr_task->wait_chain)) {
        DEBUG_DATA("WAIT EVENT ID=", curr_task->wait_event.id);
        DEBUG_PANIC("Curr task is already waiting on an event");
        sched_exit_restore(s);
    }

    q = &wait_queue[ev_hash];

    lock_spinlock(&wait_lock[ev_hash]);

    curr_task->wait_event.id = ev;
    curr_task->wait_event.wait_time_left = wait_time;
    enqueue_tail(q, &curr_task->wait_chain);

    unlock_spinlock(&wait_lock[ev_hash]);

    sched_task_sleep();
}

static void event_rm_wait_queue(task_t * task)
{
    rmqueue(&task->wait_chain);
    task->wait_event.id = 0;
    queue_zero(&task->wait_chain);
}

static void _event_signal(event_id_t ev, bool signal_all)
{
    queue_entry_t q, qe, prev_qe;
    task_t * wake_task, *task;
    unsigned int ev_hash = wait_hash(ev);

    lock_spinlock(&wait_lock[ev_hash]);
    q = &wait_queue[ev_hash];

    wake_task = NULL;
    queue_iter_safe(q, qe, prev_qe) {
        task = qe_chain_access(qe, task_t, wait_chain);
        if (!TASK_VALID(task)) {
            DEBUG_PANIC("TASK IS NOT VALID");
        }

        if (task->wait_event.id == ev) {
            sched_task_wakeup(task);
            event_rm_wait_queue(task);
            if (!signal_all)
                break;
        }
    }

    unlock_spinlock(&wait_lock[ev_hash]);
}

void event_signal(event_id_t ev)
{
    uint64_t s = sched_enter_save();
    uint32_t ev_hash = wait_hash(ev);

    if (ev_hash == NULL_EVENT_HASH) {
        sched_exit_restore(s);
    }

    _event_signal(ev, false);

    sched_exit_restore(s);
}

void event_signalall(event_id_t ev)
{
    uint64_t s = sched_enter_save();
    uint32_t ev_hash = wait_hash(ev);

    if (ev_hash == NULL_EVENT_HASH) {
        sched_exit_restore(s);
    }

    _event_signal(ev, true);
    
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

        sched_add_readyqueue(curr_task, curr_task->starting_prio);
        aarch64_dmb();
        /* Does not return, SCHED LOCK held intentional. */
        task_switch_async();
    }

    /* If the current task is not paused, continue out of ISR normally
     * since we probably had another thread scheduled. */
    sched_exit_restore(s);
}

static void sched_ready_queue_tick()
{
    queue_t qe;
    queue_t qe_prev;
    task_t * task;
    uint64_t s = sched_enter_save();

    lock_spinlock(&ready_lock);

    // No need to tick threads in the first priority queue
    for (int i = READY_QUEUE_NUM - 1; i > 0; i--) {
        queue_iter_safe(&ready_queue[i], qe, qe_prev) {
            task = qe_chain_access(qe, task_t, sched_chain);

            if (!TASK_VALID(task)) {
                DEBUG_PANIC("RQ CHAIN NOT VALID");
            }

            /* This task has been waiting on the run queue for SCHED_WAIT_TICKS,
             * elavate the priority. */
            if (task->sched_wait_ticks == 0) {
                rmqueue(&task->sched_chain);
                enqueue_tail(&ready_queue[i - 1], &task->sched_chain);
                task->sched_wait_ticks = SCHED_WAIT_TICKS;
                continue;
            }

            task->sched_wait_ticks--;
        }
    }

    unlock_spinlock(&ready_lock);

    sched_exit_restore(s);
}

static void sched_wait_queue_tick(time_us_t elapsed)
{
    queue_t q, qe, qe_prev;
    task_t * task;

    for (int i = 0; i < WAIT_QUEUE_NUM; i++) {
        q = &wait_queue[i];
        lock_spinlock(&wait_lock[i]);
        queue_iter_safe(q, qe, qe_prev) {
            task = qe_chain_access(qe, task_t, wait_chain);

            if (task->wait_event.wait_time_left) {
                if (task->wait_event.wait_time_left <= elapsed) {
                    sched_task_wakeup(task);
                    event_rm_wait_queue(task);
                } else {
                    task->wait_event.wait_time_left -= elapsed;
                }         
            }
        }

        unlock_spinlock(&wait_lock[i]);
    }
}

/* ISR Context - Called by localtimer IRQ
 * Core0 pauses the scheduler and bills time for all running threads.
 */
void sched_timer_isr()
{
    task_t * task;
    time_us_t elapsed;
    uint64_t s = sched_enter_save();

    elapsed = timer_difference(last_sched_timestamp, localtimer_gettime());
    last_sched_timestamp = localtimer_gettime();

    if (!sched_ready) {
        sched_exit_restore(s);
        return;
    }

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
            task->first_quanta = false;
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

    sched_ready_queue_tick();
    sched_wait_queue_tick(elapsed);

    aarch64_dmb();

    sched_exit_restore(s);
}


/* Select a next task to run. Called from task_switching context.
 * SCHED LOCK HELD */
task_t * sched_task_select()
{
    task_t * next_task;

    lock_spinlock(&ready_lock);
    next_task = sched_pop_readyqueue();
    unlock_spinlock(&ready_lock);
    
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

    if (!task->wait_event.id || !(task->state & TASK_WAITING)) {
        DEBUG_PANIC("No wait event or task is not in waiting state");
    }

    lock_spinlock(&task->lock);
    task->state &= ~TASK_WAITING;
    task->state |= TASK_READY;
    unlock_spinlock(&task->lock);
    
    lock_spinlock(&ready_lock);
    sched_add_readyqueue(task, task->starting_prio);
    unlock_spinlock(&ready_lock);
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

    if (!curr_task->wait_event.id) {
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

        lock_spinlock(&ready_lock);
        sched_add_readyqueue(curr_task, curr_task->starting_prio);
        unlock_spinlock(&ready_lock);


        curr_task->state &= ~TASK_RUNNING;
        curr_task->state |= TASK_READY;
    }

    sched_schedule();
}

void sched_task_add(task_t * task, task_state_t start_state, unsigned int starting_prio)
{
    sched_enter();

    if (!TASK_VALID(task)) {
        DEBUG_PANIC("TASK IS MALFORMED");
    }

    task->state = TASK_READY & start_state;
    task->starting_prio = starting_prio;
    task_reload(task);
    
    sched_add_readyqueue(task, task->starting_prio);

    sched_exit();
}

static void sched_test(void * code_addr)
{
    task_t * task;

    for (int i = 0; i < CORE_NUM; i++) {
        task = (task_t *)kalloc_alloc(sizeof(task_t), 0);
        task_create(task, code_addr);
        sched_task_add(task, 0, READY_QUEUE_NUM - 1);
    }
}

void sched_init()
{
    cpu_info_t * cpu;
    task_t * task;
    cpu_init_info();

    for (int i = 0; i < READY_QUEUE_NUM; i++) {
        queue_init(&ready_queue[i]);
    }

    spinlock_init(&ready_lock);

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

    #define TEST_NUM 2
    for (int i = 0; i < TEST_NUM; i++) {
        sched_test(test_loop);
        sched_test(test_loop2);
    }
    slock_init(&test_lock);

    sched_ready = 1;
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

