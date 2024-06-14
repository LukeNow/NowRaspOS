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
#include <kernel/kalloc.h>
#include <common/rand.h>
#include <common/bits.h>

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

void _print_linker_addrs()
{
	printfdata(".text start=", (uint64_t)__text_start);
	printfdata(".rodata start=", (uint64_t)__rodata_start);
	printfdata(".data start=", (uint64_t)__data_start);
	printfdata(".bss start=", (uint64_t)__bss_start);
	printfdata(".stackpage start=", (uint64_t)__stackpage_start);
	printfdata("earlypage start=", (uint64_t)__earlypage_start);
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

void _al_btree_verify_alloc(al_btree_t * btree, al_btree_scan_t * scan, unsigned int al_level, int expected_scan_count, int expected_level_status)
{
	unsigned int empty = al_btree_level_is_empty(btree, al_level);
	unsigned int full = al_btree_level_is_full(btree, al_level);

	DEBUG_FUNC_DIGIT("Al_btree verify alloc at al_level=", al_level);
	DEBUG_FUNC_DIGIT("Is full=", full);
	DEBUG_FUNC_DIGIT("Is empty=", empty);
	DEBUG_FUNC_DIGIT("Expected level status==", expected_level_status);
	DEBUG_FUNC_DIGIT("Expected scan_count=", expected_scan_count);
	DEBUG_FUNC_DIGIT("Actual scan count=", scan->level_count[al_level]);
	ASSERT_PANIC(scan->level_count[al_level] == expected_scan_count, "level count invalid");
	if (expected_level_status == AL_BTREE_STATUS_EMPTY) {
		ASSERT_PANIC(empty, "Expected empty is not true");
	} else if (expected_level_status == AL_BTREE_STATUS_FULL) {
		ASSERT_PANIC(full, "Expected full is not true");
	} else {
		ASSERT_PANIC(!full && !empty, "Expected partial is not true");
	}
}

void _al_btree_test()
{
	DEBUG("-----AL BTREE TEST START");
	#define AL_TEST_LEVEL_5_NUM_ALLOC 32

	#define AL_TEST_INDEX_SIZE AL_TEST_LEVEL_5_NUM_ALLOC
	
	int ret = 0;
	int indexs[AL_TEST_INDEX_SIZE];
	al_btree_t btree;
	uint8_t * buf = NULL;
	al_btree_scan_t scan;
	al_btree_scan_t count_scan;
	unsigned int index_count = 0;

	memset(&count_scan, 0, sizeof(al_btree_scan_t));
	al_btree_init(&btree);

	unsigned int al_level = AL_BTREE_MAX_LEVEL;
	int count = 1;

	DEBUG("add 1");

	memset(&scan, 0, sizeof(al_btree_scan_t));
	indexs[index_count] = al_btree_add_node(&btree, &scan, al_level);
	ASSERT_PANIC(indexs[index_count]!= -1, "Add node failed");
	index_count++;
	
	DEBUG_DATA_BINARY("AL_BTREE BUF[0]=", *(uint32_t*)(&btree));
	DEBUG_DATA_BINARY("AL_BTREE BUF[1]=", ((uint32_t*)&btree)[1]);
	
	al_btree_scan_merge(&count_scan, &scan);

	for (int i = 0; i < AL_BTREE_LEVEL_NUM; i++) {
		if (i == 0) {
			_al_btree_verify_alloc(&btree, &count_scan, i, 1, AL_BTREE_STATUS_FULL);
		} else {
			_al_btree_verify_alloc(&btree, &count_scan, i, 1, AL_BTREE_STATUS_PARTIAL);
		}
	}

	DEBUG("add 2");

	memset(&scan, 0, sizeof(al_btree_scan_t));
	indexs[index_count] = al_btree_add_node(&btree, &scan, al_level);
	ASSERT_PANIC(indexs[index_count]!= -1, "Add node failed");
	index_count++;

	al_btree_scan_merge(&count_scan, &scan);

	for (int i = 0; i < AL_BTREE_LEVEL_NUM; i++) {
		if (i == AL_BTREE_MAX_LEVEL) {
			_al_btree_verify_alloc(&btree, &count_scan, i, 2, AL_BTREE_STATUS_PARTIAL);
		} else if (i == 0) {
			_al_btree_verify_alloc(&btree, &count_scan, i, 1, AL_BTREE_STATUS_FULL);
		} else {
			_al_btree_verify_alloc(&btree, &count_scan, i, 1, AL_BTREE_STATUS_PARTIAL);
		}
	}

	DEBUG("Remove 1");
	memset(&scan, 0, sizeof(al_btree_scan_t));
	ret = al_btree_remove_node(&btree, &scan, indexs[0]);
	ASSERT_PANIC(ret != -1, "Add node failed");

	al_btree_scan_merge(&count_scan, &scan);

	for (int i = 0; i < AL_BTREE_LEVEL_NUM; i++) {
		if (i == 0) {
			_al_btree_verify_alloc(&btree, &count_scan, i, 1, AL_BTREE_STATUS_FULL);
		} else {
			_al_btree_verify_alloc(&btree, &count_scan, i, 1, AL_BTREE_STATUS_PARTIAL);
		}
	}

	DEBUG("Remove 2");
	memset(&scan, 0, sizeof(al_btree_scan_t));
	ret = al_btree_remove_node(&btree, &scan, indexs[1]);
	ASSERT_PANIC(ret != -1, "Add node failed");

	al_btree_scan_merge(&count_scan, &scan);

	for (int i = 0; i < AL_BTREE_LEVEL_NUM; i++) {
		_al_btree_verify_alloc(&btree, &count_scan, i, 0, AL_BTREE_STATUS_EMPTY);
	}

	index_count = 0;
	al_level = AL_BTREE_MAX_LEVEL;
	for (int i = 0; i < AL_TEST_LEVEL_5_NUM_ALLOC; i++) {
		memset(&scan, 0, sizeof(al_btree_scan_t));
		indexs[index_count] = al_btree_add_node(&btree, &scan, al_level);
		ASSERT_PANIC(indexs[index_count]!= -1, "Add node failed");
		al_btree_scan_merge(&count_scan, &scan);
		
		if (i != AL_TEST_LEVEL_5_NUM_ALLOC - 1)
			_al_btree_verify_alloc(&btree, &count_scan, al_level, i + 1, AL_BTREE_STATUS_PARTIAL);

		index_count++;
	}

	for (int i = 0; i < AL_BTREE_LEVEL_NUM; i++)  {
		_al_btree_verify_alloc(&btree, &count_scan, i, 1 << i, AL_BTREE_STATUS_FULL);
	}
	
	DEBUG_DATA_BINARY("AL_BTREE BUF[0]=", *(uint32_t*)(&btree));
	DEBUG_DATA_BINARY("AL_BTREE BUF[1]=", ((uint32_t*)&btree)[1]);
	
	DEBUG("-----AL BTREE TEST END");
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
	DEBUG_DATA("pool.buckets[0].array_list=", pool.buckets[0].btree);

}

void _string_test()
{
	char str[16];

	char * s = "HELLO\n";

	memset(str, 0, 16);
	memcpy(str, s, 6);

	printf(str);

}

void _kalloc_validate_alloc(uint8_t *addr, uint64_t expected, uint8_t expected_level, size_t expected_size)
{	
	DEBUG_DATA("Validating addr=", addr);
	uint8_t al_level;
	size_t alloc_size;
	unsigned int bucket_level;
	//DEBUG_DATA("validating alloc at addr=", addr);
	kalloc_header_t * header = (kalloc_header_t*)(addr - sizeof(kalloc_header_t));
	size_t val_size;
	kalloc_memnode_t * memnode = kalloc_get_memnode_from_addr(addr);
	ASSERT_PANIC(memnode, "Could not find memnode, bad addr");

	kalloc_small_bucket_t * small_bucket = kalloc_get_small_bucket_from_addr(memnode, addr);
	
	if (small_bucket)  {
		ASSERT_PANIC(header->magic == KALLOC_MAGIC, "!!!Header does not equal magic value");
		ASSERT_PANIC(header->size <= KALLOC_PAGE_BUCKET_MIN_BLOCK_SIZE, "Alloc size not in correct range");
		al_level = kalloc_size_to_bucket_level(header->size);
		DEBUG_DATA_DIGIT("Expected level =", expected_level);
		DEBUG_DATA_DIGIT("Found level =", al_level);
		ASSERT_PANIC(al_level == expected_level, "Levels do not match");
		alloc_size = header->size;
		val_size = header->size - sizeof(kalloc_header_t);
		ASSERT_PANIC(val_size == expected_size, "Expected size not equal");
	} else {
		al_level = kalloc_get_al_level_from_addr(addr);
		DEBUG_DATA_DIGIT("Expected level =", expected_level);
		DEBUG_DATA_DIGIT("Found level =", al_level);
		ASSERT_PANIC(al_level == expected_level, "Levels do not match");
		alloc_size = kalloc_bucket_level_to_size(al_level);
		val_size = expected_size;
	}

	unsigned int vals_equal = 1;
	uint64_t addr_count = (uint64_t)addr;
	for (int i = 0; i < val_size; i+= sizeof(uint64_t)) {
		if (*(uint64_t*)(addr_count + i) != expected) {
			vals_equal == 0;
			break;
		}
	}

	ASSERT_PANIC(vals_equal, "!!!Alloc expectes does not match");
}

void _kalloc_test()
{
	#define KALLOC_TEST_PAGE_NUM 16
	#define KALLOC_TEST_NUM ((PAGE_SIZE * KALLOC_TEST_PAGE_NUM )/8)
	#define KALLOC_TEST_LEVEL_PAGE_NUM (1 + KALLOC_TEST_NUM/PAGE_SIZE)
	#define KALLOC_TEST_ALLOC_WEIGHT 2
	#define KALLOC_TEST_SMALL_ALLOC_WEIGHT 5
	#define KALLOC_TEST_LEVEL_UPPER_BOUND 2
	#define KALLOC_TEST_SIZE_OFFSET 32

	DEBUG("KALLOC TEST_START");
	DEBUG_DATA("KALLOC_TEST_NUM=", KALLOC_TEST_NUM);

	uint64_t * vals = mm_earlypage_alloc(KALLOC_TEST_PAGE_NUM);
	uint64_t ** addrs = mm_earlypage_alloc(KALLOC_TEST_PAGE_NUM);
	uint8_t * levels = mm_earlypage_alloc(KALLOC_TEST_LEVEL_PAGE_NUM);
	size_t * sizes = mm_earlypage_alloc(KALLOC_TEST_PAGE_NUM);
	memset(levels, 0, KALLOC_TEST_LEVEL_PAGE_NUM);
	int ret = 0;
	
	unsigned int free = 0;
	unsigned int alloc = 0;

	uint64_t max_allocs[KALLOC_BUCKET_LEVEL_NUM];
	memset(max_allocs, 0, sizeof(max_allocs));
	uint64_t curr_allocs[KALLOC_BUCKET_LEVEL_NUM];
	memset(curr_allocs, 0, sizeof(curr_allocs));
	uint64_t small_bucket_allocs[KALLOC_BUCKET_LEVEL_NUM];
	memset(small_bucket_allocs, 0, sizeof(small_bucket_allocs));

	unsigned int level_allocs[KALLOC_BUCKET_LEVEL_NUM];
	memset(level_allocs, 0, sizeof(level_allocs));
	memset(vals, 0, KALLOC_TEST_PAGE_NUM * PAGE_SIZE);
	
	for (int i = 0; i < KALLOC_TEST_NUM; i++) {
		vals[i] = rand_prng();
	}

	for (int i = 0; i < KALLOC_TEST_NUM; i++) {
		size_t alloc_size;
		unsigned int alloc_level;

		unsigned int rand = rand_prng() % (KALLOC_TEST_ALLOC_WEIGHT + 1);
		unsigned int rand_alloc_type = rand_prng() % (KALLOC_TEST_SMALL_ALLOC_WEIGHT + 1);
		//unsigned int rand_size = ((rand_prng() % KALLOC_BUCKET_MAX_AL_LEVEL) + 1) + KALLOC_SMALL_BUCKET_LEVEL_START;
		unsigned int rand_size = (rand_prng() % (KALLOC_BUCKET_AL_LEVEL_NUM - KALLOC_TEST_LEVEL_UPPER_BOUND) + KALLOC_TEST_LEVEL_UPPER_BOUND);
		DEBUG_DATA_DIGIT("Kalloc test i=", i);

		if (rand < KALLOC_TEST_ALLOC_WEIGHT) {
			if (rand_alloc_type < KALLOC_TEST_SMALL_ALLOC_WEIGHT) {
				rand_size = rand_size + KALLOC_SMALL_BUCKET_LEVEL_START;
				alloc_size = kalloc_bucket_level_to_size(rand_size) - KALLOC_TEST_SIZE_OFFSET;
			} else {
				alloc_size = kalloc_bucket_level_to_size(rand_size);
			}

			DEBUG_DATA_DIGIT("----------------ALLOC OF SIZE=", alloc_size);
			addrs[alloc] = kalloc_alloc(alloc_size, 0);
			ASSERT_PANIC(addrs[alloc] != NULL, "!!!alloc failed");
			sizes[alloc] = alloc_size;
			for (int i = 0; i < alloc_size; i+= sizeof(uint64_t)) {
				*(uint64_t*)((uint64_t)addrs[alloc] + i) = vals[alloc];
			}

			/*
			if (rand_size == KALLOC_SMALL_BUCKET_LEVEL_START) {
				alloc_level = 5;
			} else if (rand_size > KALLOC_SMALL_BUCKET_LEVEL_START) {
				alloc_level = rand_size - 1;
			} else {
				alloc_level = rand_size;
			} */

			levels[alloc] = rand_size;
			level_allocs[rand_size]++;
			alloc++;
		} else if (alloc > free) {
			DEBUG("----------------FREE");
			_kalloc_validate_alloc(addrs[free], vals[free], levels[free], sizes[free]);
			ret = kalloc_free(addrs[free], 0);
			ASSERT_PANIC(ret == 0, "!!!free failed");
			free++;
		}
	}

	// Free the remaining allocs
	for (int i = free; i < alloc; i++) {
		_kalloc_validate_alloc(addrs[free], vals[free], levels[free], sizes[free]);
		ret = kalloc_free(addrs[free], 0);
		ASSERT_PANIC(ret == 0, "!!!free failed");
		free++;
	}

	for (int i = 0; i < KALLOC_BUCKET_LEVEL_NUM; i++) {
		DEBUG_FUNC_DIGIT("Totals allocs at level level=", i);
		DEBUG_FUNC_DIGIT("Total allocs=", level_allocs[i]);
	}

	// Get the actual totals vs theoretical totals to make sure everything is actually free
	kalloc_get_total_allocs(curr_allocs, max_allocs, small_bucket_allocs);
	for (int i = 0; i < KALLOC_BUCKET_LEVEL_NUM; i++) {
		DEBUG_FUNC_DIGIT("Totals vs maxs level i=", i);
		DEBUG_FUNC_DIGIT("Curr_allocs[i]=", curr_allocs[i]);
		DEBUG_FUNC_DIGIT("Max_allocs[i]=", max_allocs[i]);
		DEBUG_FUNC_DIGIT("Small bucket allocs[i]= ", small_bucket_allocs[i]);
	}

	DEBUG("--KALLOC TEST DONE");
}

void kernel_main(uint64_t dtb_ptr32, uint64_t x1, uint64_t x2, uint64_t x3)
{
	uint32_t mem_size;
	uint32_t mem_base_addr;
	uint8_t *page_p;
	int r;

	_zero_mem();
	uart_init();

	_print_linker_addrs();
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

	mm_early_init();

	_ll_test();
	_math_test();
	_al_btree_test();
	_string_test();
	_pool_test();
	kalloc_init((uint64_t)__earlypage_end);
	_kalloc_test();
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