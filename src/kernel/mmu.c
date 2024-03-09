#include <stdint.h>
#include <stddef.h>
#include <kernel/uart.h>
#include <common/assert.h>
#include <kernel/kern_defs.h>
#include <kernel/addr_defs.h>
#include <kernel/mm.h>
#include <kernel/gpio.h>

#define PT_ENTRY    0b11
#define PT_PAGE     0b11     
#define PT_BLOCK    0b01    

/* Lower Attributes */
// bits[63:52, 11:2] are identical for both block and entry descriptors
// level 0, and level 1 entries cannot be block descriptors
// accessibility

#define PT_EL0_RW   0b010


#define PT_KERNEL   (0<<6)      // privileged, supervisor EL1 access only
#define PT_USER     (1<<6)      // unprivileged, EL0 access allowed
#define PT_RW       (0<<7)      // read-write
#define PT_RO       (1<<7)      // read-only
#define PT_AF       (1<<10)     // accessed flag
#define PT_NX       (1UL<<54)   // no execute
// shareability
#define PT_OSH      (2<<8)      // outter shareable
#define PT_ISH      (3<<8)      // inner shareable
// indexes to be defined in MAIR register
#define PT_MEM      (0<<2)      // normal memory
#define PT_DEV      (1<<2)      // device MMIO
#define PT_NC       (2<<2)      // non-cachable

#define TTBR_CNP    1

#define PAGE_OFFSET_MASK (0xFFF)
#define PAGE_MASK (~0xFFF)
#define UPPER_BITS_MASK (0xFFFF000000000000) //Check the upper bit addr?

// MAP MMIO block upper to lower half

#define BLOCK_SIZE MB(2)
#define ENTRIES_PER_PAGE 512

#define PAGE_OFF 14
#define PAGE_MASK BITS(PAGE_OFF)
#define L3_BIT_OFF 12
#define L2_BIT_OFF 21
#define L1_BIT_OFF 30
#define L0_BIT_OFF 39
#define L3_MASK (BITS(9) << L3_BIT_OFF)
#define L2_MASK (BITS(9) << L2_BIT_OFF)
#define L1_MASK (BITS(9) << L1_BIT_OFF)
#define L0_MASK (BITS(9) << L0_BIT_OFF)
#define ADDR_MASK (BITS(48) & ~PAGE_MASK)

static uint64_t high_l0_entry = 0;
static uint64_t low_l0_entry = 0;
static uint64_t *high_l1_table_start;
static uint64_t *low_l1_table_start;

#define MEM_LIMIT 32 //in GB

/* We will have a 38 Bit addressable Mem range = 512GB. 
 * Start L1 map contingous mem with 32GB Addressable mem. */

static uint32_t table_size = 0;

static int mmu_map_entry(uint64_t *phys_addr, uint64_t *virt_addr, uint64_t attributes)
{   

    uint64_t *l0_tbl = NULL;
    uint64_t *l1_tbl = NULL;
    uint64_t *l2_tbl = NULL;
    uint64_t *l3_tbl = NULL;
    uint64_t *virt_addr_normalized = (uint64_t*)((uint64_t)virt_addr & ~UPPER_BITS_MASK);
    uint64_t *phys_addr_normalized = (uint64_t*)((uint64_t)phys_addr  & ~PAGE_MASK);

    int PT_PRIV = 0;
    /*
    printf("MMU_MAP_ENTRY: phys =");
    uart_hex(phys_addr);
    printf(" virt_addr= ");
    uart_hex(virt_addr);
    printf("\n");
    */

    if  ((uint64_t)virt_addr_normalized & L0_MASK) {
        printf("L0 MASK non zero above addressable range\n");
        return 1;
    }

    if ((uint64_t)virt_addr & UPPER_BITS_MASK) {
        l1_tbl = (uint64_t *)high_l1_table_start;
        PT_PRIV = PT_KERNEL;
    } else {
        l1_tbl = (uint64_t *)low_l1_table_start;
        PT_PRIV = PT_USER;
    }

    ASSERT(l1_tbl);

    uint64_t l1_table_off = (uint64_t)virt_addr_normalized >> L1_BIT_OFF;
    uint64_t l2_table_off = (uint64_t)virt_addr_normalized >> L2_BIT_OFF;
    uint64_t l3_table_off = (uint64_t)virt_addr_normalized >> L3_BIT_OFF;

    int l2_tbl_idx = l2_table_off % ENTRIES_PER_PAGE;
    int l3_tbl_idx = l3_table_off % ENTRIES_PER_PAGE;



    /*
    printf("l1_table_off= ");
    uart_hex(l1_table_off);
    printf(" l2_table_off= ");
    uart_hex(l2_table_off);
    printf(" l3_table_off= ");
    uart_hex(l3_table_off);
    printf("\n");
    */

    if (l1_table_off >= 32) {
        printf("Addressing greater than 32GB\n");
        return 1;
    }

     /* The L1 will point to the first 1GB block of memory */
    if (l1_tbl[l1_table_off] == 0)  {
        if ((l2_tbl = mm_earlypage_alloc(1)) == NULL) {
            printf("l2_table alloc failed\n");
            return 1;
        }

        l1_tbl[l1_table_off] = (uint64_t)l2_tbl |
                                            PT_ENTRY |
                                            PT_AF |
                                            PT_PRIV|
                                            PT_ISH |
                                            PT_MEM;
    }

    l2_tbl = (uint64_t *) (l1_tbl[l1_table_off] & PAGE_MASK); //0 out upper bits?
    
    if (l2_tbl[l2_tbl_idx] == 0) {
        /* This is a memory block descriptor of 2MB, */
        if (attributes & PT_BLOCK) {
            if(!IS_ALIGNED((uint64_t)virt_addr_normalized, MB(2))) {
                printf("PT_BLOCK VIRT ADDR NOT ALIGNED\n");
                ASSERT(0);
                return 1;
            }
            
            l2_tbl[l2_tbl_idx] = (uint64_t)phys_addr_normalized |
                                            PT_BLOCK |
                                            PT_PRIV |
                                            attributes;
            return 0;
        }

        if ((l3_tbl = mm_earlypage_alloc(1)) == NULL) {
            printf("l3_table alloc failed\n");
            return 1;
        }

        l2_tbl[l2_tbl_idx] = (uint64_t)l3_tbl |
                                            PT_ENTRY |
                                            PT_AF |
                                            PT_PRIV |
                                            PT_ISH |
                                            PT_MEM;
    }

    l3_tbl = (uint64_t *) (l2_tbl[l2_tbl_idx] & PAGE_MASK);
    /* The L3 table entries might not be contiguous since we will alloc
     * them on the fly. */
    if (l3_tbl[l3_tbl_idx] == 0)  {
        l3_tbl[l3_tbl_idx] = (uint64_t)phys_addr_normalized |
                                                    PT_PAGE |
                                                    PT_PRIV |
                                                    attributes;
                                                    
    }
    
   // printf("VIRT ADDR ");
   // uart_hex((uint64_t) virt_addr);
   // printf("\n");
   // printf("PHYS ADDR ");
   // uart_hex((uint64_t) phys_addr);
   // printf("\n");


    ASSERT(l1_tbl && l2_tbl  && l3_tbl);

    return 0;
}


int mmu_init()
{
    uint64_t r;
    uint64_t attr = 0;
    uint64_t *addr = NULL;

    printf("MMU INIT ENTER\n");
    /* 32 GB addr space for high and low mem. */
    high_l1_table_start = mm_earlypage_alloc(1);
    low_l1_table_start = mm_earlypage_alloc(1);

    printf("==MMU INIT==\n");
    printf("high_l1 addr: ");
    uart_hex(high_l1_table_start);
    printf("\n");

    printf("low_l1 addr: ");
    uart_hex(low_l1_table_start);
    printf("\n");


    for (int i = BLOCK_SIZE; i < MB(64); i+= BLOCK_SIZE) {
         addr = (uint64_t *)i;
    }


    for (int i = 0; i < MB(4); i += PAGE_SIZE) {
        attr = 0;
        addr = (uint64_t *)i;

        attr = PT_AF | PT_NX;
        if  (i >= MMIO_BASE && i <= MMIO_BASE + PAGE_SIZE) {
            attr |= PT_OSH | PT_DEV;
           // printf("Hit MMIOU ADDR: ");
           // uart_hex(i);
          //  printf("\n");
        } else {
            attr |= PT_ISH | PT_MEM;
        }

        r = mmu_map_entry(addr, addr, attr);
        if (r != 0) {
             printf("Map fail on virt addr");
            uart_hex(addr);
            printf("\n");
            ASSERT(0);
            return 1;
        } 
        

        r = mmu_map_entry(addr, ((uint64_t)addr) + UPPER_ADDR_MASK, attr);
        if (r != 0) {
            printf("Map fail on UPPER virt addr");
            uart_hex(((uint64_t)addr) + UPPER_ADDR_MASK);
            printf("\n");
            ASSERT(0);
            return 1;
        } 
        
    }

    //printf("MMIO BASE MAPPING\n");
    attr = PT_AF | PT_NX | PT_OSH | PT_DEV | PT_PAGE;
    addr = (uint64_t *)MMIO_BASE;
    for (int i = 0; i < KB(16); i += PAGE_SIZE) {

        r = mmu_map_entry(addr, addr, attr);
        if (r != 0) {
             printf("Map fail on virt addr");
            uart_hex(addr);
            printf("\n");
            ASSERT(0);
            return 1;
        } 
        

        r = mmu_map_entry(addr, ((uint64_t)addr) + UPPER_ADDR_MASK, attr);
        if (r != 0) {
            printf("Map fail on UPPER virt addr");
            uart_hex(((uint64_t)addr) + UPPER_ADDR_MASK);
            printf("\n");
            ASSERT(0);
            return 1;
        } 

        addr += i;

    }

    printf("MAPPING DONE\n");
    r = 0;

    asm volatile ("mrs %0, id_aa64mmfr0_el1" : "=r" (r));
    uint64_t b = r & 0xF;
    if (b < 1) {
        printf("Atleast 36 bit phys addr size not supported\n");
        ASSERT(0);
        return 1;
    }

    // first, set Memory Attributes array, indexed by PT_MEM, PT_DEV, PT_NC in our example
    r=  (0xFF << 0) |    // AttrIdx=0: normal, IWBWA, OWBWA, NTR
        (0x04 << 8) |    // AttrIdx=1: device, nGnRE (must be OSH too)
        (0x44 <<16);     // AttrIdx=2: non cacheable
    asm volatile ("msr mair_el1, %0" : : "r" (r));
    printf("MAPPING 1\n");
    r = 0;
    // next, specify mapping characteristics in translate control register
    r=  (0b00LL << 37) | // TBI=0, no tagging
        (b << 32) |      // IPS=autodetected
        (0b10LL << 30) | // TG1=4k
        (0b11LL << 28) | // SH1=3 inner
        (0b01LL << 26) | // ORGN1=1 write back
        (0b01LL << 24) | // IRGN1=1 write back
        (0b0LL  << 23) | // EPD1 enable higher half
        (25LL   << 16) | // T1SZ=25, 3 levels (512G)
        (0b00LL << 14) | // TG0=4k
        (0b11LL << 12) | // SH0=3 inner
        (0b01LL << 10) | // ORGN0=1 write back
        (0b01LL << 8) |  // IRGN0=1 write back
        (0b0LL  << 7) |  // EPD0 enable lower half
        (25LL   << 0);   // T0SZ=25, 3 levels (512G)
    asm volatile ("msr tcr_el1, %0; isb" : : "r" (r));
printf("MAPPING 2\n");
       // tell the MMU where our translation tables are. TTBR_CNP bit not documented, but required
    // lower half, user space
    asm volatile ("msr ttbr0_el1, %0" : : "r" (((uint64_t)low_l1_table_start) + 1));
    // upper half, kernel space
    asm volatile ("msr ttbr1_el1, %0" : : "r" (((uint64_t)high_l1_table_start) + 1));

    
    r = 0;
printf("MAPPING 3\n");
    // finally, toggle some bits in system control register to enable page translation
    asm volatile ("dsb ish; isb; mrs %0, sctlr_el1" : "=r" (r));
    r|=0xC00800;     // set mandatory reserved bits
    r&=~((1<<25) |   // clear EE, little endian translation tables
         (1<<24) |   // clear E0E
         (1<<19) |   // clear WXN
         (1<<12) |   // clear I, no instruction cache
         (1<<4) |    // clear SA0
         (1<<3) |    // clear SA
         (1<<2) |    // clear C, no cache at all
         (1<<1));    // clear A, no aligment check
    r|=  (1<<0);     // set M, enable MMU
    printf("OK\n");
    asm volatile ("msr sctlr_el1, %0; isb" : : "r" (r));


    printf("==MMU INIT OK==\n");

    return 0;
}