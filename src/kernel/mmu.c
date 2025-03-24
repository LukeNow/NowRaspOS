#include <stdint.h>
#include <stddef.h>
#include <kernel/uart.h>
#include <common/assert.h>
#include <kernel/addr_defs.h>
#include <kernel/mm.h>
#include <kernel/gpio.h>
#include <common/bits.h>
#include <common/aarch64_common.h>
#include <kernel/mmu.h>
#include <common/assert.h>

uint64_t mmu_gpu_to_arm_addr(uint64_t gpu_bus_addr)
{
    return gpu_bus_addr &= ~GPU_DMA_BASE;
}

uint64_t mmu_arm_to_gpu_addr(uint64_t arm_addr)
{
    if (arm_addr >= MMU_UPPER_ADDRESS) {
        arm_addr = mmu_get_phys_addr(arm_addr);
    }

    return arm_addr |= GPU_DMA_BASE;
}

uint64_t mmu_get_phys_addr(uint64_t virt_addr)
{
    if (virt_addr < MMU_UPPER_ADDRESS)
        return virt_addr;
    
    // TODO actually check the page table for this addr, for now this is okay.
    return virt_addr - MMU_UPPER_ADDRESS;
}

uint64_t mmu_get_kern_addr(uint64_t phys_addr)
{
    if (phys_addr >= MMU_UPPER_ADDRESS)
        return phys_addr;
    
    // TODO actually check the page table for this addr, for now this is okay.
    return phys_addr + MMU_UPPER_ADDRESS;

}


int mmu_map_entry(uint64_t *phys_addr, uint64_t *virt_addr, uint64_t attributes)
{

}


int mmu_init(uint32_t phy_mem_size, uint32_t vc_mem_start, uint32_t vc_mem_size)
{

}
