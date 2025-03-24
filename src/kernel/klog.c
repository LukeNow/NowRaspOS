#include <stdint.h>
#include <kernel/klog.h>
#include <kernel/kalloc.h>
#include <common/aarch64_common.h>
#include <common/atomic.h>
#include <common/assert.h>
#include <emb-stdio/emb-stdio.h>
#include <kernel/cpu.h>
#include <kernel/task.h>
#include <kernel/irq.h>
#include <kernel/sched.h>
#include <kernel/timer.h>

/* TIMESTAMP: PROCESSOR_ID: TASK_ID: MSG*/
#define MSG_PROLOGUE "%u:%u:%u: %s"

#define WRITE_FULL(r_i, w_i) (r_i == (w_i + 1 % KLOG_BUF_SIZE))
#define READ_FULL(r_i, w_i) (r_i == w_i)
#define END_INDEX(i) (i == KLOG_BUF_SIZE - 1)

static klog_t * log;


/*
 * w_i - first index that is free to write to
 * r_i - the last index that was read
 * start - w_i == 0, r_i == KLOG_BUF_SIZE
 * FULL - r_i == w_i
 * NON_EMPTY - r_i behind w_i
 * 
 * */


static int reserve()
{   
    uint64_t w_i;
    uint64_t r_i;
    int ret;

    do {
        r_i = atomic_ld_64(&log->read_index);
        w_i = atomic_ld_64(&log->write_index);
        
        /* FULL */
        if (WRITE_FULL(r_i, w_i)) {
            return -1;
        }
        
        if (END_INDEX(w_i)) {
            ret = atomic_cmpxchg_64(&log->write_index, w_i, 0);
        } else {
            ret = atomic_cmpxchg_64(&log->write_index, w_i, w_i + 1);
        }
    } while (ret);

    return w_i;
}

static int flush()
{
    uint64_t w_i;
    uint64_t r_i;
    int ret;

    if (!log->handler) {
        return 1;
    }

    w_i = atomic_ld_64(&log->write_index);
    r_i = atomic_ld_64(&log->read_index);
    
    while (!READ_FULL(r_i, w_i)) {
        log->handler(&log->msgs[r_i].msg);

        if (END_INDEX(r_i)) {
            r_i = 0;
        } else {
            r_i ++;
        }

    }

    atomic_str_64(&log->read_index, r_i);
    return 0;
}

void klog_init_handler(void(*handler) (char*))
{
    log->handler = handler;
}

static void klog_flush_task()
{
    int ret;
    ticks_t ticks;
    while (1) {

        rwlock_write_lock(&log->rw_lock);
        
        ticks = localtimer_getticks();

        ret = flush();

        rwlock_write_unlock(&log->rw_lock);

        sched_yield();
    }
}

static int klog_write(char *s, size_t strlen)
{
    int index;

    rwlock_read_lock(&log->rw_lock);

    index = reserve();
    if (index < 0) {
        rwlock_read_unlock(&log->rw_lock);
        return 1;
    }

    memcpy(&log->msgs[index].msg[0], s, strlen);

    rwlock_read_unlock(&log->rw_lock);
    aarch64_dmb();

    return 0;
}

int klog_printf(const char* fmt, ...)
{
    ticks_t timer_ticks;
    cpu_info_t * cpu;
    int ret;
    va_list args;	
    char str[KLOG_MSG_SIZE];
    char msg[KLOG_MSG_SIZE];

    va_start(args, fmt);

    stdio_vsnprintf(&str[0], KLOG_MSG_SIZE, fmt, args);

    cpu = cpu_get_currcpu_info();
    timer_ticks = localtimer_getticks();
    stdio_snprintf(&msg[0], KLOG_MSG_SIZE, MSG_PROLOGUE, timer_ticks, cpu->cpu_id, cpu->curr_task->task_id, &str[0]);
    ret = klog_write(&msg[0], KLOG_MSG_SIZE);

    va_end(args);

    return ret;
}

void klog_init(void(*handler) (char*))
{ 
    task_t * task;

    log = kalloc_alloc(sizeof(klog_t), 0);
    if (!log) {
        DEBUG_PANIC("Klog init failed");
    }
    memset(log, 0, sizeof(klog_t));

    rwlock_init(&log->rw_lock, KLOG_BUF_SIZE);
    log->read_index = 0;
    log->write_index = 0;
    log->handler = NULL;

    klog_init_handler(handler);

    task = (task_t *)kalloc_alloc(sizeof(task_t), 0);
    task_create(task, klog_flush_task);
    sched_task_add(task, TASK_UNINT, 0);

    log->flush_task = task;
}
