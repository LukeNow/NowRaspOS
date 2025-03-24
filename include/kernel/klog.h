#ifndef __KLOG_H
#define __KLOG_H

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

#include <common/lock.h>
#include <kernel/task.h>

#define KLOG_MSG_SIZE 128
#define KLOG_BUF_SIZE (256 * 2 * 2)

typedef struct {
    char msg[KLOG_MSG_SIZE];
} klog_msg_t;

typedef struct {
    uint64_t read_index;
    uint64_t write_index;
    rw_lock_t rw_lock;
    void(*handler) (char*);
    task_t * flush_task;
    klog_msg_t msgs[KLOG_BUF_SIZE];
} klog_t  __attribute__ ((aligned (8)));

void klog_init(void(*handler) (char*));
int klog_printf(const char* fmt, ...);
void klog_init_handler(void(*handler) (char*));

#endif