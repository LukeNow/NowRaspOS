#include <stddef.h>
#include <stdint.h>
 
#include <kernel/uart.h>
#include <kernel/mbox.h>
#include <kernel/mmu.h>
#include <kernel/lock.h>
#include <common/assert.h>
#include <common/common.h>
#include <common/math.h>
#include <kernel/atomic.h>
#include <kernel/aarch64_common.h>
#include <kernel/addr_defs.h>
#include <kernel/kern_defs.h>
#include <kernel/irq.h>
#include <kernel/mm.h>
#include <common/arraylist_btree.h>
#include <common/string.h>
#include <kernel/poolalloc.h>

#define CORE_NUM 4

#define UPPER_ADDR 0xFFFF000000000000

ATOMIC_UINT32(atomic_r);
ATOMIC_UINT32(core_ready);

DEFINE_SPINLOCK(uart_lock);

static void _mmu_test()
{
	uint32_t s = 1;

	uint32_t *s_upper = (uint32_t *)((uint64_t)(&s) + UPPER_ADDR);

	printf("LETS TRY\n");
	uart_hex(&s);
	printf("\n");

	printf("UPPER ADDR\n");
	uart_hex(((uint64_t)(&s) + UPPER_ADDR));
	printf("\n");
	*s_upper = 2;
	printf("\n");
	printf("DONE!\n");
	lock_spinunlock(&uart_lock);
}

static void _zero_mem()
{
	uint32_t end = ((uint32_t)__earlypage_end);
	uint32_t start = ((uint32_t)__stackpage_start);
	uint32_t size = end - start;

	for (uint32_t *i = ((uint32_t *)start); i < ((uint32_t *)end); i ++) {
		*i = 0;
	}
}


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
	uint32_t  __attribute__((aligned(16))) mbox[36];

	//uint32_t *mbox = mm_earlypage_alloc(1);
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
	msg = mbox_make_msg(&mbox, MBOX_CH_PROP);
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

#include <common/linkedlist.h>
LL_ROOT_INIT(ll_root);

static void _ll_test()
{
	int ret = 0;
	ll_node_t * temp;

	LL_ROOT_ZERO(ll_root);
	ll_node_t node1;
	node1.data = (void*)1;

	ll_node_t node2;
	node2.data = (void*)2;

	ll_node_t node3;
	node3.data = (void*)3;

	ll_node_t node4;
	node4.data = (void*)4;

	ll_node_t node5;
	node5.data = (void*)5;

	ll_node_t node6;
	node6.data = (void*)6;

	ll_push_list(&ll_root, &node1);
	ll_push_list(&ll_root, &node2);

	ll_traverse_list(&ll_root);
	/*
	if (ll_node_exists(&ll_root, &node1)) {
		printf("EXISTS FAILED\n");
	}


	if (ret = ll_delete_node(&ll_root, &node1)) {
		printf("DELETE FAILED\n");
	}
	*/
	//ll_traverse_list(&ll_root);
	temp = ll_pop_list(&ll_root);

	printf("POPPED NODE: ");
	uart_hex((unsigned int)temp->data);
	printf("\n");

	ll_push_list(&ll_root, &node2);

	ll_insert_node(&ll_root, &node2, &node3);

	ll_traverse_list(&ll_root);

	ll_append_list_index(&ll_root, &node4, 0);

	ll_traverse_list(&ll_root);

	ll_append_list_index(&ll_root, &node5, -1);

	ll_traverse_list(&ll_root);

	ll_append_list_index(&ll_root, &node6, -2);

	ll_traverse_list(&ll_root);

	ll_delete_list_index(&ll_root, 0);

	ll_traverse_list(&ll_root);
	
}

void _math_test()
{
	printf("MATH TEST START\n");
	int ret = 0;
	ret = math_log2_64(64);
	printf("math_log2(64)\n");
	uart_hex(ret);
	printf("\n");
	ASSERT(ret == 6);

	ret = math_log2_64(8);
	printf("math_log2(8)\n");
	ASSERT(ret == 3);

	ret = math_log2_64(9);
	printf("math_log2(9)\n");
	ASSERT(ret == 3);


	ASSERT(math_is_power2_64(2 * 2));
	ASSERT(math_is_power2_64(1));
	ASSERT(math_is_power2_64(1024));
	ASSERT(!math_is_power2_64(1025));

	printf("MATH TEST DONE\n");
}

void _al_btree_test()
{
	#define AL_TEST_SIZE 64
	#define INDEX_SIZE (AL_TEST_SIZE / 2)
	
	int ret = 0;
	int indexs[INDEX_SIZE];
	al_btree_t tree;
	uint8_t * buf = NULL;

	#define LEVEL 3
	#define NUM 3
	

	printf("---BTREE TEST START---\n");
	al_btree_init(&tree);

	for (int i = 0; i < INDEX_SIZE; i++) {

		ret = al_btree_add_node(&tree, -1);
		if (ret == -1) {
			printf("AL_BTREE_ADD_LEAF FAILED\n");
			CYCLE_INFINITE;
		}

		printfdigit("NODE INDEX=", ret);

		indexs[i] = ret;
	}

	for (int i = 0; i < INDEX_SIZE; i++) {
		DEBUG_DATA_DIGIT("Removing index=", indexs[i]);
		ret = al_btree_remove_node(&tree, indexs[i]);
		if (ret) {
			printf("AL_BTREE_REMOVEW_LEAF FAILED\n");
			CYCLE_INFINITE;
		}
	}


	printfbinary("BUFF: ", *(uint32_t*)(&tree.list));
	printfbinary("BUFF: ", ((uint32_t*)(&tree.list))[1]);
	//printfhex("BUFF: ", *(uint32_t *)&buf[4]);
	//printfbinary("BUFF: ", *(uint32_t *)&buf[8]);
	//printfbinary("BUFF: ", *(uint32_t *)&buf[12]);
	
	printf("---BTREE TEST END---\n");

}

void _pool_test()
{
	int ret = 0;

	#define OBJECT_TEST_SIZE 64
	uint8_t buf[OBJECT_TEST_SIZE];
	poolalloc_bucket_t buckets[OBJECT_TEST_SIZE];
	uint64_t *objects[OBJECT_TEST_SIZE];
	unsigned int object_count = 0;
	memset(objects, 0, sizeof(uint64_t *) * OBJECT_TEST_SIZE);

	uint8_t * object;

	poolalloc_t pool;
	poolalloc_init(&pool, OBJECT_TEST_SIZE, 1, &buf[0], &buckets[0]);

	DEBUG_DATA("&BUF[0]=", &buf[0]);

	for (int i = 0; i < OBJECT_TEST_SIZE; i++) {
		DEBUG_FUNC_DIGIT("Poolalloc test i=", i);
		
		object = poolalloc_alloc(&pool);
		ASSERT(object);

		objects[object_count] = object;
		object_count++;
	}


	for (int i = 0; i < OBJECT_TEST_SIZE; i++) {
		ret = poolalloc_free(&pool, objects[i]);
		if (ret) {
			ASSERT(0);
		}
	}
	DEBUG_DATA("pool.buckets[0].array_list=", pool.buckets[0].btree.list);

}

void _string_test()
{
	char str[16];

	char * s = "HELLO\n";

	memset(str, 0, 16);
	memcpy(str, s, 6);

	printf(str);

}
void kernel_main(uint64_t dtb_ptr32, uint64_t x1, uint64_t x2, uint64_t x3)
{
	uint32_t mem_size;
	uint32_t mem_base_addr;
	uint8_t *page_p;
	int r;

	uart_init();

	_print_processor_features();
	_zero_mem();

	atomic_add(&atomic_r, 1);
	ASSERT(atomic_r == 1);

	atomic_add (&atomic_r, 2);
	ASSERT(atomic_r == 3);

	lock_spinlock(&uart_lock);
	uart_hex(&atomic_r);
	printf("\n");
	printf("Hello from main core!\n");


	lock_spinunlock(&uart_lock);

	mm_early_init();

	_ll_test();
	_math_test();
	//_al_btree_test();
	_string_test();
	_pool_test();
	//printf("==IRQ INIT==\n");
	//irq_init();
	//mmu_init();
	
	lock_spinlock(&uart_lock);

	uart_hex(dtb_ptr32);

	uart_puts("Hello worlds\n");
	uart_puts("Hello world again\n");	

	_get_mem_size(&mem_base_addr, &mem_size);
	printfdata("MEMSIZE in pages?=", mem_size);
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

	//_mmu_test();

	while (1) {
		CYCLE_WAIT(10);
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