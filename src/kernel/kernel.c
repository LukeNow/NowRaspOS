#include <stddef.h>
#include <stdint.h>
 
#include <kernel/uart.h>
#include <kernel/mbox.h>
#include <kernel/mmu.h>
#include <kernel/lock.h>
#include <common/assert.h>
#include <common/common.h>
#include <kernel/atomic.h>
#include <kernel/aarch64_common.h>
#include <kernel/addr_defs.h>
#include <kernel/kern_defs.h>
#include <kernel/irq.h>
#include <kernel/mm.h>

#define CORE_NUM 4

#define UPPER_ADDR 0xFFFF000000000000

ATOMIC_UINT32(atomic_r);
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
	volatile uint8_t *wake_addr = 0xE0;
	for(int i = 1; i < 4; i++)  {
		*(uint64_t*)wake_addr = &_start;
		wake_addr += 8; 
	}
	aarch64_syncb();
	/* wake the cores that have been sleeping*/
	aarch64_sev();

}

static void _get_mem_size(uint32_t *base_addr, uint32_t *size)
{
	//uint32_t  __attribute__((aligned(16))) mbox[36];

	uint32_t *mbox = mm_earlypage_alloc(1);
	mbox_message_t msg;
	printf("ENTERTING MEMSIZE\n");
    mbox[0] = 8*4;                  // length of the message
    mbox[1] = MBOX_REQUEST;         // this is a request message
    mbox[2] = MBOX_TAG_VCMEM;   
    mbox[3] = 8;                    // buffer size
    mbox[4] = 8;
    mbox[5] = 0;                    // clear output buffer
    mbox[6] = 0;
    mbox[7] = MBOX_TAG_LAST;

	printf("middle memsize\n");
	msg = mbox_make_msg(&mbox, MBOX_CH_PROP);
	mbox_send(msg, MBOX_CH_PROP);
	printf("middle memsize\n");
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

void kernel_main(uint64_t dtb_ptr32, uint64_t x1, uint64_t x2, uint64_t x3)
{
	uint32_t mem_size;
	uint32_t mem_base_addr;
	uint8_t *page_p;
	int r;

	uart_init();

	_print_processor_features();

	atomic_add(&atomic_r, 1);
	ASSERT(atomic_r == 1);

	atomic_add (&atomic_r, 2);
	ASSERT(atomic_r == 3);

	lock_spinlock(&uart_lock);
	uart_hex(&atomic_r);
	printf("\n");
	printf("Hello from main core!\n");


	lock_spinunlock(&uart_lock);

	/*
	while (1) {
		CYCLE_WAIT(5);
	}
	*/

	//_start_cores();
	mm_early_init();
	
	/*
	lock_t lock;
	r = lock_trylock(&lock);
	ASSERT(r == 0);
	r = lock_trylock(&lock);
	ASSERT(r == 1);
	lock_tryunlock(&lock);
	r = lock_trylock(&lock);
	ASSERT(r == 0);
	*/

	printf("==IRQ INIT==\n");
	irq_init();
	mmu_init();
	
	lock_spinlock(&uart_lock);

	uart_hex(dtb_ptr32);

	uart_puts("Hello worlds\n");
	uart_puts("Hello world again\n");	

	_get_mem_size(&mem_base_addr, &mem_size);

	//uart_puts("Base address: ");
	//uart_hex(mem_base_addr);
	//uart_puts("\n");
	//uart_puts("Mem size in bytes: ");
	//uart_hex(mem_size);
	//uart_puts("\n");
	//printf("MEM_SIZE in pages: ");
	//uart_hex(mem_size / PAGE_SIZE);
	//printf("\n");

	ASSERT(IS_ALIGNED(8, 4));
	ASSERT(!IS_ALIGNED(9, 4));
	ASSERT(IS_ALIGNED(mem_base_addr, PAGE_SIZE));
	ASSERT(!IS_ALIGNED(mem_base_addr  + 1, PAGE_SIZE));

	printf("\n");
	uart_hex(BITS(1));
	printf("\n");
	uart_hex(BITS(2));
	printf("\n");
	uart_hex(BITS(0));
	printf("\n");
	uart_hex(BITS(PAGE_BIT_OFF));
	printf("\n");

	ASSERT(IS_ALIGNED(PAGE_SIZE, BITS(PAGE_BIT_OFF) + 1));
	ASSERT(IS_BIT_ALIGNED(PAGE_SIZE, PAGE_BIT_OFF));

	/*
	page_p = mm_earlypage_alloc(1);
	ASSERT((uint64_t)page_p == (uint64_t)__earlypage_start);

	page_p = mm_earlypage_alloc(3);
	ASSERT((uint64_t)page_p == ((uint64_t)__earlypage_start + PAGE_SIZE));

	page_p = mm_earlypage_alloc(1);
	ASSERT((uint64_t)page_p ==  (uint64_t)__earlypage_start + (PAGE_SIZE  * 4));
	*/


	uint32_t s = 1;

	uint32_t *s_upper = (uint32_t *)((uint64_t)(&s) + UPPER_ADDR);

	printf("LETS TRY\n");


	uart_hex(&s);



	printf("\n");

	printf("UPPER ADDR\n");
	uart_hex(((uint64_t)(&s) + UPPER_ADDR));
	printf("\n");
	
	_break();
	*s_upper = 2;
	printf("\n");
	printf("DONE!\n");

	lock_spinunlock(&uart_lock);
	
	/*
	while (1) {
		CYCLE_WAIT(10);
		if (lock_trylock(&uart_lock) != 0)
			continue;

		//printf("CORE READY= ");
		//uart_hex(core_ready);
		//printf("\n");
		lock_tryunlock(&uart_lock);
	}
	*/


	while (1) {
		uart_send(uart_getc());
	}
}