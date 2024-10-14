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
#include <common/arraylist_btree.h>
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

#define CORE_NUM 4

ATOMIC_UINT32(core_ready);
DEFINE_SPINLOCK(uart_lock);

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
	int corenum = mpidr_el1  & 0xFF;

	while (!uart_isready()) {
		CYCLE_WAIT(5);
	}

	atomic_add((uint64_t *)&core_ready, 1);

	irq_init();

	lock_spinlock(&uart_lock);
	printf("Corenum: ");
	uart_hex(corenum);
	printf("\n");
	lock_spinunlock(&uart_lock);


	while (1) {
		CYCLE_WAIT(10);
	}

}

static void start_cores(uint64_t core_start_addr)
{
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

void kernel_main(uint64_t dtb_ptr32, uint64_t core_start_addr, uint64_t x2, uint64_t x3)
{
	uint32_t vc_mem_size;
	uint32_t vc_base_addr;
	uint32_t mem_size;
	uint32_t mem_base_addr;
	uint8_t *page_p;
	int r;

	uart_init();

	printf("Hello from main core!\n");

	irq_init();

	timer_init();
	timer_enable();

	mm_init();
	kalloc_init();

	ll_test();
	kalloc_slab_test();
	kalloc_cache_test();
	mm_test();
	kalloc_test();
	
	start_cores(core_start_addr);

	DEBUG_PANIC("END core0");
	
	DEBUG("--- KERNEL END ---");

	while (1) {

		//DEBUG_DATA_DIGIT("Timer num=", timer_get_time());
		CYCLE_WAIT(1000);
		if (lock_trylock(&uart_lock) != 0)
			continue;

		
		lock_tryunlock(&uart_lock);
	}
	


	while (1) {
		uart_send(uart_getc());
	}
}