#include <stddef.h>
#include <stdint.h>
#include <kernel/uart.h>
#include <kernel/mbox.h>
#include <kernel/mmu.h>
#include <common/lock.h>
#include <common/assert.h>
#include <common/common.h>
#include <common/math.h>
#include <common/atomic.h>
#include <common/aarch64_common.h>
#include <kernel/addr_defs.h>
#include <kernel/irq.h>
#include <kernel/mm.h>
#include <common/string.h>
#include <kernel/kalloc.h>
#include <common/rand.h>
#include <common/bits.h>
#include <kernel/kalloc_slab.h>
#include <kernel/kalloc_cache.h>
#include <kernel/kalloc.h>
#include <kernel/kalloc_page.h>
#include <kernel/kern_tests.h>
#include <kernel/timer.h>
#include <kernel/kalloc_page.h>
#include <kernel/sched.h>
#include <kernel/task.h>
#include <kernel/cpu.h>
#include <kernel/early_mm.h>
#define CORE_NUM 4

ATOMIC_UINT64(core_ready);
DEFINE_SPINLOCK(uart_lock);

#define LOCALTIMER_PERIOD 10000

static void _print_processor_features()
{
	uint64_t r = 0;

	asm volatile ("mrs %0, id_aa64mmfr0_el1" : "=r" (r));

	if ((r & 0xF) >= 1) {
		printf("Atleast 36bit physical address bus supported\n");
	} else {
		printf("36 bit physicall address bus not supported\n");
	}

    if (r & (0b111 << 20))  {
        printf("16KB granulatory supported\n");
    } else {
        printf("16KB granulatory not supported\n"); 
    }

    if ((r & (0b111 << 24) == 0))  {
        printf("64kbKB granulatory supported\n");
    } else {
        printf("64kbKB granulatory not supported\n"); 
    }

     if ((r & (0b111 << 28)) == 0)  {
        printf("4kbKB granulatory supported\n");
    } else {
        printf("4KB granulatory not supported\n"); 
    }
}

void kernel_child_main(uint64_t mpidr_el1)
{
	uint64_t core;
	int corenum = mpidr_el1  & 0xFF;

	while (!uart_isready()) {
		CYCLE_WAIT(5);
	}

	core = atomic_fetch_add_64((uint64_t *)&core_ready, 1);

	DEBUG_DATA_DIGIT("Seen cores=", core);

	irq_init();
	mbox_enable_irq(corenum);
	while (atomic_ld_64(&core_ready) != CORE_NUM - 1) {
		CYCLE_WAIT(10);
	}
	
	DEBUG_DATA_DIGIT("Core num start=", corenum);


	
	sched_start();


	while (1) {
		CYCLE_WAIT(10);
	}

	//localtimer_irqinit(LOCALTIMER_PERIOD, corenum);

}

static void start_cores(uint64_t core_start_addr)
{
	aarch64_dsb();
	/* Write start addr to location the cores are waiting on. */
	volatile uint8_t *wake_addr = (volatile uint8_t*)0xE0;
	for(int i = 1; i < 4; i++)  {
		*(uint64_t*)wake_addr = core_start_addr;
		wake_addr += 8; 
	}
	aarch64_dsb();
	/* wake the cores that have been sleeping*/
	aarch64_sev();
}

void notfunc()
{
	DEBUG("ENTERED");
}


void kernel_main(uint64_t dtb_ptr32, uint64_t core_start_addr, uint64_t x2, uint64_t x3)
{
	uint64_t time;
	uint32_t vc_mem_size;
	uint32_t vc_base_addr;
	uint32_t mem_size;
	uint32_t mem_base_addr;
	uint8_t *page_p;
	mmu_mem_map_t * phys_mem_map;
	uint32_t * dtb;
	int r;

	uint32_t buff[5];

	uart_init();

	phys_mem_map = mm_early_get_memmap();

	dtb = (uint32_t *)dtb_ptr32;
	DEBUG_DATA("CORE START ADDR=", core_start_addr);
	DEBUG_DATA("DTB PTR=", dtb);
	DEBUG_DATA("dtb = ", *dtb);

    DEBUG("Print memmap");
    for (int i = 0; i < 4; i++) {
        DEBUG_DATA("Start = ", phys_mem_map[i].start_addr);
        DEBUG_DATA("Size =", phys_mem_map[i].size);
        DEBUG_DATA("End addr =", phys_mem_map[i].start_addr + phys_mem_map[i].size);
    }
	//uart_init();

	//printf("Hello from main core!\n");


	
	r = mbox_request(&buff[0], 5, MAILBOX_TAG_GET_ARM_MEMORY, 8, 8, 0, 0);
	if (r) {
		DEBUG_PANIC("Mem request failed");
	}

	DEBUG_DATA("Membase addr=", buff[3]);
	DEBUG_DATA("Memsize= ", buff[4]);

	r = mbox_request(&buff[0], 5, MAILBOX_TAG_GET_VC_MEMORY, 8, 8, 0, 0);
	if (r) {
		DEBUG_PANIC("Mem request failed");
	}
	
	DEBUG_DATA("VC Membase addr=", buff[3]);
	DEBUG_DATA("VC Memsize= ", buff[4]);

	mm_init();
	kalloc_init();
	/*
	ll_test();
	kalloc_slab_test();
	kalloc_cache_test();
	mm_test();
	kalloc_test();
	*/

	/*
	for (int i =0; i < 10; i++) {
		systemtimer_wait(1000000);
		time = systemtimer_gettime_64();

		DEBUG_DATA_DIGIT("Time = ", time);
	} */
	


	sched_init();
	
	irq_init();
	mbox_enable_irq(0);
	//DEBUG_PANIC("END core0");

	localtimer_irqinit(LOCALTIMER_PERIOD, 0);
	start_cores(core_start_addr);

	while (atomic_ld_64(&core_ready) != CORE_NUM - 1) {
		CYCLE_WAIT(10);
	}
	
	DEBUG("CORE0 Start");
	//systemtimer_initirq(100000);
	//localtimer_irqinit(LOCALTIMER_PERIOD, 0);
	


	sched_start();

//	DEBUG_PANIC("END core0");
	

	
	DEBUG_PANIC("END core0");
	DEBUG("--- KERNEL END ---");


	while (1) {
		uart_send(uart_getc());
	}
}