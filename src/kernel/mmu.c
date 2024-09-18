#include <stdint.h>
#include <stddef.h>
#include <kernel/uart.h>
#include <common/assert.h>
#include <kernel/kern_defs.h>
#include <kernel/addr_defs.h>
#include <kernel/mm.h>
#include <kernel/gpio.h>
#include <common/bits.h>
#include <kernel/aarch64_common.h>
#include <kernel/mmu.h>
#include <common/assert.h>

#define PT_ENTRY    0b11
#define PT_PAGE     0b11     
#define PT_BLOCK    0b01    

#define LEVEL1_BLOCKSIZE (1 << 21)

#define PAGE_OFFSET_MASK (0xFFF)
#define PAGE_MASK (~0xFFF)
#define UPPER_BITS_MASK (0xFFFF000000000000) //Check the upper bit addr?

#define ENTRIES_PER_PAGE 512

#define PAGE_OFF 12
#define PAGE_MASK BITS(PAGE_OFF)

#define L2_BIT_OFF 12
#define L1_BIT_OFF 21
#define L0_BIT_OFF 30

#define L2_MASK (BITS(9) << L2_BIT_OFF)
#define L1_MASK (BITS(9) << L1_BIT_OFF)
#define L0_MASK (BITS(9) << L0_BIT_OFF)

#define L2_ENTRY(ADDR) ((((ADDR) & L2_MASK) >> L2_BIT_OFF) % ENTRIES_PER_PAGE)
#define L1_ENTRY(ADDR) ((((ADDR) & L1_MASK) >> L1_BIT_OFF) % ENTRIES_PER_PAGE)
#define L0_ENTRY(ADDR) (((ADDR) & L0_MASK) >> L0_BIT_OFF)

#define PAGE_NORMALIZED_ADDR(ADDR) ((ADDR) / PAGE_SIZE)
#define BLOCK_NORMALIED_ADDR(ADDR) ((ADDR) / LEVEL1_BLOCKSIZE)

#define ADDR_MASK (BITS(39) & ~PAGE_MASK)

static uint64_t *high_l0_entry;
static uint64_t *low_l0_entry;

static map_entry(uint64_t addr, uint64_t attrs)
{
    uint64_t entry_l1;
    uint64_t entry_l0;
    uint64_t * entry_l1_addr;
    uint32_t l0_index;
    uint32_t l1_index;

    l0_index = L0_ENTRY(addr);

    DEBUG_DATA("Mapping addr=", addr);

    entry_l0 = low_l0_entry[l0_index];

    if (entry_l0 == 0) {
        entry_l1_addr = (uint64_t *)mm_earlypage_alloc(1);
        memset(entry_l1_addr, 0, PAGE_SIZE);
        low_l0_entry[l0_index] = PT_SECURE | (uint64_t)entry_l1_addr | PT_ENTRY;
    }

    entry_l1_addr = (uint64_t *)(entry_l0 & PT_ADDR);
    l1_index = L1_ENTRY(addr);

    DEBUG_DATA("entry_l1_addr=", entry_l1_addr);
    DEBUG_DATA_DIGIT("l0_index=", l0_index);
    DEBUG_DATA_DIGIT("l1_index=", l1_index);

    entry_l1 = entry_l1_addr[l1_index];

    if (entry_l1 == 0) {
        entry_l1 |= attrs |
                    PT_ACCESSED | 
                    PT_BLOCK | 
                    addr;

        entry_l1_addr[l1_index] = entry_l1;
    }
}

/* We will have a 38 Bit addressable Mem range = 512GB. 
 * Start L1 map contingous mem with 32GB Addressable mem. */
int mmu_init(uint32_t phy_mem_size, uint32_t vc_mem_start, uint32_t vc_mem_size)
{
    uint32_t vc_block_size = vc_mem_size / LEVEL1_BLOCKSIZE;
    uint64_t attr = 0;
    uint64_t addr = 0;
    uint64_t reg = 0;
    uint64_t r = 0;
    uint64_t id = 0;

    uint64_t * low_l1_entry;

    low_l0_entry = (uint64_t *)mm_earlypage_alloc(1);
    low_l1_entry = (uint64_t *)mm_earlypage_alloc(1);
    memset(low_l0_entry, 0, PAGE_SIZE);
    memset(low_l1_entry, 0, PAGE_SIZE);

    low_l0_entry[0] = PT_SECURE | (uint64_t)low_l1_entry | PT_ENTRY;
    
    uint64_t vc_ram_start = vc_mem_start / LEVEL1_BLOCKSIZE;
    for (uint64_t i = 0; i < vc_ram_start; i++) {
        map_entry(i * LEVEL1_BLOCKSIZE, PT_MEM_ATTR(MT_NORMAL) | PT_INNER_SHAREABLE);
    }

    uint64_t vc_ram_end = (MMIO_BASE) / LEVEL1_BLOCKSIZE;
    for (uint64_t i = vc_ram_start; i < vc_ram_end; i++) {
        map_entry(i * LEVEL1_BLOCKSIZE, PT_MEM_ATTR(MT_NORMAL_NC));
    }

    uint64_t vc_mem_end = ((vc_mem_size - (MMIO_BASE - vc_mem_start)) + MMIO_BASE) / LEVEL1_BLOCKSIZE;
    for (uint64_t i = vc_ram_end; i < vc_mem_end; i++) {
        map_entry(i * LEVEL1_BLOCKSIZE, PT_MEM_ATTR(MT_DEVICE_NGNRNE));
    }
    
    // Full system data barrier, make sure all writes are commited
    aarch64_dsb();
    // first, set Memory Attributes array, indexed by PT_MEM, PT_DEV, PT_NC in our example
    r = MAIR1_VAL;

    AARCH64_MSR(mair_el1, r);

    AARCH64_MSR(ttbr0_el1, (uint64_t)low_l0_entry);
    AARCH64_MSR(ttbr1_el1, (uint64_t)low_l0_entry);

    AARCH64_MRS(id_aa64mmfr0_el1, id);

    aarch64_isb();
    //DEBUG_DATA_DIGIT("mem model features=",id);
    
    r = 0;
    r =  (0b00LL << 38) | // TBI1=0, Use top byte for address calculation for TTBR1_EL1
        (0b00LL << 37) | // TBI0=0, Use top byte for address calculation for TTBR0_EL1
        (0b0LL  << 36) | // AS, ASID size, 8 bit default
        (id & 0b111 << 32) | // IPS size
        (0b10LL << 30) | // TG1=4k granulatity
        (0b11LL << 28) | // SH1=3 Sharability for inner mem only
        (0b01LL << 26) | // ORGN1=1 same as below but for outer memory
        (0b01LL << 24) | // IRGN1=1 write back write-allocate cacheable on page table inner mem for TTBR1_EL1
        (0b0LL  << 23) | // EPD1, enable table walks on tlb miss for TTBR1_EL1
        (0b0LL  << 22) | // A1, TTBR0_EL1 defines the ASID values
        (25LL   << 16) | // T1SZ=25, 3 levels (512G) for ttbr1_el1
        (0b00LL << 14) | // TG0=4k granularity
        (0b11LL << 12) | // SH0=3 Sharability for inner mem only
        (0b01LL << 10) | // ORGN0=1 same as below but for outer memory
        (0b01LL << 8) |  // IRGN0=1 write back write-allocate cacheable on on page table inner memory
        (0b0LL  << 7) |  // EPD0, enable table walks on TLB miss for TTBR0_EL1
        (25LL   << 0);   // T0SZ=25, 3 levels (512G), for ttbr0_el1
    
    AARCH64_MSR(tcr_el1, r);

    aarch64_isb();

    r = 0;
    AARCH64_MRS(sctlr_el1, r);

    r |= SCTLREL1_VAL;

    AARCH64_MSR(sctlr_el1, r);
 
    aarch64_isb();

    DEBUG("--- MMU INIT DONE ---");

    return 0;
}

/*
static int mmu_map_entry(uint64_t *phys_addr, uint64_t *virt_addr, uint64_t attributes)
{

} */