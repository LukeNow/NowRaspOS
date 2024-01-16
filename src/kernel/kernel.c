#include <stddef.h>
#include <stdint.h>
 
#include <kernel/uart.h>
#include <kernel/mbox.h>
#include <kernel/mmu.h>
#include <kernel/lock.h>
#include <common/assert.h>
#include <common/common.h>
#include <kernel/atomic.h>

DEFINE_SPINLOCK(uart_lock);

void kernel_child_main(uint64_t corenum)
{

	while (!uart_isready()) {
		CYCLE_WAIT(5);
	}

	


}

static void _get_mem_size(uint32_t *base_addr, uint32_t *size)
{
	uint32_t  __attribute__((aligned(16))) mbox[36];
	mbox_message_t msg;

    mbox[0] = 8*4;                  // length of the message
    mbox[1] = MBOX_REQUEST;         // this is a request message
    mbox[2] = MBOX_TAG_VCMEM;   
    mbox[3] = 8;                    // buffer size
    mbox[4] = 8;
    mbox[5] = 0;                    // clear output buffer
    mbox[6] = 0;
    mbox[7] = MBOX_TAG_LAST;

	msg = mbox_make_msg(&mbox, MBOX_CH_PROP);
	mbox_send(msg, MBOX_CH_PROP);

	*base_addr = 0;
	*size = 0;
	
	if (mbox_read(&mbox[0], MBOX_CH_PROP)) {
		*base_addr = mbox[5];
		*size = mbox[6];
	}
}

void kernel_main(uint64_t dtb_ptr32, uint64_t x1, uint64_t x2, uint64_t x3)
{
	uint32_t mem_size;
	uint32_t mem_base_addr;
	int r;

	uart_init();
	lock_spinlock(&uart_lock);
	printf("Hello from main core!");

	lock_spinunlock(&uart_lock);

	/*
	lock_t lock;
	r = lock_trylock(&lock);
	uart_init();
	ASSERT(r == 0);
	r = lock_trylock(&lock);
	ASSERT(r == 1);
	lock_tryunlock(&lock);
	r = lock_trylock(&lock);
	ASSERT(r == 0);
	*/

	/*
	uart_hex(dtb_ptr32);

	uart_puts("Hello worlds\n");
	uart_puts("Hello world again\n");	

	_get_mem_size(&mem_base_addr, &mem_size);

	uart_puts("Base address: ");
	uart_hex(mem_base_addr);
	uart_puts("\n");
	uart_puts("Mem size in bytes: ");
	uart_hex(mem_size);
	uart_puts("\n");

	mmu_init();
	*/

	while (1) {
		CYCLE_WAIT(5);
	}



	while (1) {
		uart_send(uart_getc());
	}
}