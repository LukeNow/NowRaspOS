
#include <stddef.h>
#include <stdint.h>
#include <common/assert.h>
#include <kernel/cpu.h>
#include <common/aarch64_common.h>
#include <kernel/kalloc.h>
#include <common/string.h>
#include <common/lock.h>
#include <emb-stdio/emb-stdio.h>
#include <kernel/mbox.h>

DEFINE_SPINLOCK(dump_lock);

cpu_info_t per_cpu_info[CORE_NUM];

uint32_t cpu_get_id()
{   
    uint32_t id;
    
    AARCH64_MRS(mpidr_el1, id);

    return id & 3;
}

cpu_info_t * cpu_get_percpu_info(uint32_t id)
{
    return &per_cpu_info[id];
}

cpu_info_t * cpu_get_currcpu_info()
{
    return &per_cpu_info[cpu_get_id()];
}

void cpu_core_dump_all()
{
    uint32_t cpu_id = cpu_get_id();

    for (int i = 0; i < CORE_NUM; i++) {
        if (i == cpu_id)
            continue;

        mbox_core_cmd_int(i, cpu_id, CORE_DUMP, 0);
    }

    cpu_core_dump();
}

void cpu_core_stop()
{
    while (1) {
        aarch64_wfe();
    }
}

void cpu_core_dump()
{
    uint64_t esr;
    uint64_t far;
    task_t * task;

    lock_spinlock(&dump_lock);
    stdio_printf("-----------------------------\n");
    stdio_printf("CPU ID=%d\n", cpu_get_id());
    AARCH64_MRS(esr_el1, esr);
    stdio_printf("ESR=%lx\n", esr);
    AARCH64_MRS(far_el1, far);
    stdio_printf("Far=%lx\n", far);
    stdio_printf("Failed task addr= %lx\n", cpu_get_currcpu_info()->curr_task);
    stdio_printf("Failed Stack addr= %lx\n", cpu_get_currcpu_info()->curr_task->el1_stack_ptr);
    stdio_printf("Failed code addr= %lx\n", *(uint64_t*)cpu_get_currcpu_info()->curr_task->el1_stack_ptr);
    stdio_printf("-----------------------------\n");
    unlock_spinlock(&dump_lock);

    cpu_core_stop();
}

void cpu_init_info()
{
    memset(per_cpu_info, 0, sizeof(cpu_info_t) * CORE_NUM);

    for (int i = 0; i < CORE_NUM; i++) {
        per_cpu_info[i].cpu_id = i;
    }
}