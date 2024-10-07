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
#include <common/poolalloc.h>
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

/*
static void _zero_mem()
{
	uint64_t early_page_end = ((uint64_t)__earlypage_end);
	uint64_t early_page_start = ((uint64_t)__earlypage_start);
	uint64_t early_page_size = early_page_end - early_page_start;

	uint64_t bss_end = (uint64_t)__bss_end;
	uint64_t bss_start = (uint64_t)__bss_start;
	uint64_t bss_size = bss_end - bss_start;

	uint64_t start, end;

	for (uint64_t i = bss_start; i < bss_end; i += sizeof(uint64_t)) {
		*(uint64_t*)i = 0;
	}

	for (uint64_t i = early_page_start; i < early_page_end; i += sizeof(uint64_t)) {
		*(uint64_t*)i = 0;
	}
} */

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

	atomic_add(&core_ready, 1);
	
	lock_spinlock(&uart_lock);
	printf("Corenum: ");
	uart_hex(corenum);
	printf("\n");
	lock_spinunlock(&uart_lock);


	while (1) {
		CYCLE_WAIT(10);
	}

}

static void _start_cores()
{
	/* Write start addr to location the cores are waiting on. */
	volatile uint8_t *wake_addr = (volatile uint8_t*)0xE0;
	for(int i = 1; i < 4; i++)  {
		*(uint64_t*)wake_addr = (uint64_t)&_start;
		wake_addr += 8; 
	}
	aarch64_dsb();
	/* wake the cores that have been sleeping*/
	aarch64_sev();

}

static void _get_mem_size(uint32_t *base_addr, uint32_t *size, mbox_prop_tag_t tag)
{
	uint32_t  __attribute__((aligned(16))) mbox[36];

	//uint32_t *mbox = mm_earlypage_alloc(1);
	mbox_message_t msg;
	printf("ENTERTING MEMSIZE\n");
    mbox[0] = 8*4;                  // length of the message
    mbox[1] = MBOX_REQUEST;         // this is a request message
    mbox[2] = tag;   
    mbox[3] = 8;                    // buffer size
    mbox[4] = 8;
    mbox[5] = 0;                    // clear output buffer
    mbox[6] = 0;
    mbox[7] = MBOX_TAG_LAST;
	msg = mbox_make_msg(&mbox[0], MBOX_CH_PROP);
	mbox_send(msg, MBOX_CH_PROP);
	*base_addr = 0;
	*size = 0;
	
	if (mbox_read(&mbox[0], MBOX_CH_PROP)) {
		*base_addr = mbox[5];
		*size = mbox[6];
	}
	printf("MEMSIZE DONE\n");
}

static void _break() {
	return;
}

void reserve_mem_regions()
{
	int ret = 0;
	uint64_t start_addr = 0;
	for (int i = 0; i < MM_RESERVE_AREA_INDEX; i++) {
		ret = kalloc_page_reserve_pages(start_addr, MM_MAX_ORDER, 0);
		ASSERT_PANIC(!ret, "MM reserve pages failed");
		start_addr += MM_MEMORDER_TO_PAGES(MM_MAX_ORDER) * PAGE_SIZE;
		//DEBUG_DATA_DIGIT("Startaddr=", start_addr);
		ret = kalloc_page_reserve_pages(start_addr, MM_MAX_ORDER, 0);
		ASSERT_PANIC(!ret, "MM reserve pages failed");
		start_addr += MM_MEMORDER_TO_PAGES(MM_MAX_ORDER) * PAGE_SIZE;
	}

	DEBUG("--- Initial mem regions reserved---");
}

uint64_t upper_data;

static void mmu_test()
{
	uint32_t s = 1;

	upper_data = 1;
	uint32_t *s_upper = (uint32_t *)((uint64_t)(&s) + MMU_UPPER_ADDRESS);

	DEBUG_DATA("Upper data addr=", &upper_data);
	DEBUG_DATA("Upper bits=", MMU_UPPER_ADDRESS);
	DEBUG_DATA("s_upper=", s_upper);
	DEBUG_DATA_DIGIT("Upper adder1=", *s_upper);

	*s_upper = *s_upper + 1;

	DEBUG_DATA_DIGIT("Upper adder2=", *s_upper);
}


void kernel_main(uint64_t dtb_ptr32, uint64_t x1, uint64_t x2, uint64_t x3)
{
	uint32_t vc_mem_size;
	uint32_t vc_base_addr;
	uint32_t mem_size;
	uint32_t mem_base_addr;
	uint8_t *page_p;
	int r;

	//_zero_mem();
	uart_init();

	printf("Hello from main core!\n");

	//mm_early_init();

	ll_test();

	//DEBUG_PANIC("EOK REACHED");


	_get_mem_size(&mem_base_addr, &mem_size, MAILBOX_TAG_GET_ARM_MEMORY);
	_get_mem_size(&vc_base_addr, &vc_mem_size, MAILBOX_TAG_GET_VC_MEMORY);

	irq_init();

	//DEBUG_DATA("Mem base addr=", mem_base_addr);
	//DEBUG_DATA("MEMSIZE in pages?=", mem_size);
	//DEBUG_DATA("VC mem base addr=", vc_base_addr);
	//DEBUG_DATA("VC memsize=", vc_mem_size);

	//mmu_init(mem_size, vc_base_addr, vc_mem_size);

	mmu_test();
	//DEBUG_PANIC("EOK REACHED");

	timer_init();
	timer_enable();

	mm_init(mem_size, 0);
	reserve_mem_regions();

	//kalloc_init();
	//kalloc_test();

	//kalloc_slab_test();
	//kalloc_cache_test();
	mm_test();
	kalloc_init();
	kalloc_test();

	DEBUG("--- KERNEL END ---");

	while (1) {

		//DEBUG_DATA_DIGIT("Timer num=", timer_get_time());
		CYCLE_WAIT(1000);
		if (lock_trylock(&uart_lock) != 0)
			continue;

		//printf("CORE READY= ");
		//uart_hex(core_ready);
		//printf("\n");
		lock_tryunlock(&uart_lock);
	}
	


	while (1) {
		uart_send(uart_getc());
	}
}