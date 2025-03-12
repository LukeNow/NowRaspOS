#ifndef __CPU_H
#define __CPU_H

#include <stdint.h>
#include <stddef.h>
#include <kernel/task.h>

// 4 cores on rasbi 3b+, we can later dynamically detect these if we want to target other architectures
#define CORE_NUM 4

typedef uint64_t reg_t;

typedef struct regs {
    reg_t x0;
    reg_t x1;
    reg_t x2;
    reg_t x3;
    reg_t x4;
    reg_t x5;
    reg_t x6;
    reg_t x7;
    reg_t x8;
    reg_t x9;
    reg_t x10;
    reg_t x11;
    reg_t x12;
    reg_t x13;
    reg_t x14;
    reg_t x15;
    reg_t x16;
    reg_t x17;
    reg_t x18;
    reg_t x19;
    reg_t x20;
    reg_t x21;
    reg_t x22;
    reg_t x23;
    reg_t x24;
    reg_t x25;
    reg_t x26;
    reg_t x27;
    reg_t x28;
    reg_t x29;
    reg_t x30;
} regs_t;

typedef struct info_regs {
    reg_t spsr_el1; // saved program state register
    reg_t elr_el1; // exception link register
    reg_t sp_el1; // stack ptr
} info_regs_t;

typedef struct fpu_regs {
    reg_t q0;
    reg_t q1;
    reg_t q2;
    reg_t q3;
    reg_t q4;
    reg_t q5;
    reg_t q6;
    reg_t q7;
    reg_t q8;
    reg_t q9;
    reg_t q10;
    reg_t q11;
    reg_t q12;
    reg_t q13;
    reg_t q14;
    reg_t q15;
    reg_t q16;
    reg_t q17;
    reg_t q18;
    reg_t q19;
    reg_t q20;
    reg_t q21;
    reg_t q22;
    reg_t q23;
    reg_t q24;
    reg_t q25;
    reg_t q26;
    reg_t q27;
    reg_t q28;
    reg_t q29;
    reg_t q30;
} fpu_regs_t;

typedef struct cpu_info {
    task_t * curr_task;
    uint32_t cpu_id;
} cpu_info_t __attribute__((aligned(8)));


#define CURR_TASK (cpu_get_currcpu_info()->curr_task)

uint32_t cpu_get_id();
cpu_info_t * cpu_get_percpu_info(uint32_t id);
cpu_info_t * cpu_get_currcpu_info();
void cpu_init_info();

#endif