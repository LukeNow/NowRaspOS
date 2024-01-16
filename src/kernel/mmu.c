#include <stdint.h>
#include <stddef.h>
#include <kernel/uart.h>
#include <common/assert.h>

#define PAGESIZE    4096

// granularity
#define PT_PAGE     0b11        // 4k granule
#define PT_BLOCK    0b01        // 2M granule
// accessibility
#define PT_KERNEL   (0<<6)      // privileged, supervisor EL1 access only
#define PT_USER     (1<<6)      // unprivileged, EL0 access allowed
#define PT_RW       (0<<7)      // read-write
#define PT_RO       (1<<7)      // read-only
#define PT_AF       (1<<10)     // accessed flag
#define PT_NX       (1UL<<54)   // no execute
// shareability
#define PT_OSH      (2<<8)      // outter shareable
#define PT_ISH      (3<<8)      // inner shareable
// defined in MAIR register
#define PT_MEM      (0<<2)      // normal memory
#define PT_DEV      (1<<2)      // device MMIO
#define PT_NC       (2<<2)      // non-cachable

#define TTBR_CNP    1

#define PAGE_OFFSET_MASK (0xFFF)
#define PAGE_MASK (~0xFFF)
// Start of data segment, pages before should be mapped RO
extern volatile uint8_t __data_start; 
// End of our linked data, we will store our page tables after this
extern volatile uint8_t __end;

extern volatile uint8_t __data_start;
extern volatile uint8_t __data_end;
extern volatile uint8_t __text_start;

// MAP MMIO block upper to lower half


uint64_t PAGE_OFF = (uint64_t)0x0000000000000FFF; //12 bits
uint64_t L3_OFF = (((uint64_t)0x00000000000001FF) << 12); //9 bits
uint64_t L2_OFF = (((uint64_t)0x00000000000001FF) << 21); // 9 bits
uint64_t L1_OFF = (((uint64_t)0x00000000000001FF) << 30); // 9 bits

static uint32_t table_size = 0; 

static mmu_map_entry(uint64_t *phys_addr, uint64_t *virt_addr, uint64_t flags)
{   
    /*
    uint64_t tbl_l1_off = virt_addr & L1_OFF;
    uint64_t tbl_l2_off = virt_addr & L2_OFF;
    uint64_t tbl_l3_off = virt_addr & L3_OFF;
    */
}


void mmu_init()
{

    uint64_t *table = __end + PAGESIZE;
    table_size += PAGESIZE;


    uint64_t op =  (PAGE_OFF & L3_OFF) | (L3_OFF  & L2_OFF) | (L2_OFF & L1_OFF);

    ASSERT(op == 0);
    

    //| L2_OFF | L1_OFF;
    //uint64_t op = 0xFFFFFFFFFF;
    uart_hex(op);

}