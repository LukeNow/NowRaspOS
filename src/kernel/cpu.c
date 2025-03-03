
#include <stddef.h>
#include <stdint.h>
#include <common/assert.h>
#include <kernel/cpu.h>
#include <common/aarch64_common.h>
#include <kernel/kalloc.h>
#include <common/string.h>

cpu_info_t per_cpu_info[CORE_NUM];

uint32_t cpu_get_id()
{   
    uint32_t id;
    
    AARCH64_MRS(mpidr_el1, id);

    return id & 3;
}

cpu_info_t * cpu_get_percpu_info(uint32_t id)
{
    ASSERT_PANIC(id < CORE_NUM, "ID is greated than core num");

    return &per_cpu_info[id];
}

cpu_info_t * cpu_get_currcpu_info()
{
    return &per_cpu_info[cpu_get_id()];
}


void cpu_init_info()
{

    memset(per_cpu_info, 0, sizeof(cpu_info_t) * CORE_NUM);

    for (int i = 0; i < CORE_NUM; i++) {
        per_cpu_info[i].cpu_id = i;
    }
}