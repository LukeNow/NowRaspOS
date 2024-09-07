
#include <stddef.h>
#include <stdint.h>
#include <kernel/kern_tests.h>
#include <common/common.h>
#include <common/linkedlist.h>
#include <kernel/kalloc_slab.h>
#include <kernel/kalloc_cache.h>
#include <kernel/mm.h>
#include <kernel/printf.h>
#include <kernel/kalloc.h>
#include <common/assert.h>
#include <kernel/kalloc_page.h>

void linked_list_test()
{
	int ret = 0;
	ll_node_t * temp;
    ll_node_t ll_root;

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

void math_test()
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

/*
typedef struct struct_s {
	int mem1;
	int mem2;
	uint64_t mem3;
	uint8_t mem4;
	uint32_t mem5;
} struct_s;

void test_struct_p(void)
{
	DEBUG("----- test struct p macro----");
	struct_s s;
	s.mem1 = 1;
	s.mem2 = 2;
	s.mem3 = 3;
	s.mem4 = 4;
	s.mem5 = 5;

	int * mem2_p = &s.mem2;

	struct_s * s_p = STRUCT_P(mem2_p, struct_s, mem2);

	ASSERT_PANIC(s_p->mem2 == 2, "Mem 2 !=2");
	ASSERT_PANIC(s_p->mem5 == 5, "Not equal");

}
*/

int _kalloc_slab_verify(kalloc_slab_t * slab, unsigned int index, uint32_t expected)
{
	uint32_t * ptr = (uint32_t*)((uint8_t *)slab->mem_ptr + slab->obj_size * index);
	uint32_t found = *ptr;
	DEBUG_FUNC("Addr checked at addr=", ptr);
	DEBUG_FUNC("Expected val=", expected);
	DEBUG_FUNC("Found val=", found);
	if (found != expected)
		return 1;

	return 0;
}

void kalloc_slab_test()
{	
	DEBUG("---Slab test start---");

	#define SLAB_TEST_NUM_PAGES 4
	#define SLAB_TEST_DATA_PAGES SLAB_TEST_NUM_PAGES * 6
	#define SLAB_TEST_ALLOC_WEIGHT 5
	
	int alloc;
	int free;
	int rand;
	int ret;
	uint8_t * mem =(uint8_t *) mm_earlypage_alloc(SLAB_TEST_NUM_PAGES);
	uint32_t ** ptrs = (uint32_t **)mm_earlypage_alloc(SLAB_TEST_DATA_PAGES);
	uint32_t * vals = (uint32_t *)mm_earlypage_alloc(SLAB_TEST_DATA_PAGES);
	kalloc_slab_t * slab = (kalloc_slab_t *)mm_earlypage_alloc(1);
	kalloc_slab_t * slab_ret = NULL;

	memset(ptrs, 0, PAGE_SIZE * SLAB_TEST_DATA_PAGES);
	memset(vals, 0, PAGE_SIZE  * SLAB_TEST_DATA_PAGES);
	
	unsigned int max_num = kalloc_slab_obj_num(sizeof(uint32_t), PAGE_SIZE * SLAB_TEST_NUM_PAGES);
	memset(slab, kalloc_slab_struct_size(max_num, sizeof(uint32_t)), 0);
	
	for (unsigned int i = 0; i < max_num; i++) {
		vals[i] = rand_prng();
	}

	/* Externally alloced slab test. */
	slab_ret = kalloc_slab_init(slab, mem, SLAB_TEST_NUM_PAGES, sizeof(uint32_t), 0);

	max_num = slab_ret->max_num;

	ASSERT_PANIC(slab_ret == slab, "Returned slab not the same");

	for (unsigned int i = 0; i < max_num; i++) {
		ptrs[i] = kalloc_slab_alloc(slab);
		*ptrs[i] = vals[i];
	}

	alloc = 0;
	free = 0;
	for (unsigned int i = 0; i < max_num; i++) {
		rand = rand_prng() % SLAB_TEST_ALLOC_WEIGHT;
		ASSERT_PANIC(_kalloc_slab_verify(slab, i, vals[i]) == 0, "Val not expected");
		ret = kalloc_slab_free(slab, ptrs[i]);
		free ++;

		if (rand < SLAB_TEST_ALLOC_WEIGHT && free > alloc) {
			ptrs[alloc] = kalloc_slab_alloc(slab);
			*ptrs[alloc] = vals[alloc];
			ASSERT_PANIC(_kalloc_slab_verify(slab, alloc, vals[alloc]) == 0, "Val not expected");
			alloc++;
		}
	}

	for (int i = free; i < alloc; i++) {
		ASSERT_PANIC(_kalloc_slab_verify(slab, i, vals[i]) == 0, "Val not expected");
		ret = kalloc_slab_free(slab, ptrs[i]);
		ASSERT_PANIC(!ret, "slab free failed");
	}

	/* Embedded slab test. */
	memset(ptrs, 0, PAGE_SIZE * SLAB_TEST_DATA_PAGES);
	memset(vals, 0, PAGE_SIZE * SLAB_TEST_DATA_PAGES);
	memset(slab, 0, PAGE_SIZE);
	memset(mem, 0, PAGE_SIZE * SLAB_TEST_NUM_PAGES);

	slab_ret = kalloc_slab_init(NULL, mem, SLAB_TEST_NUM_PAGES, sizeof(uint32_t), 0);
	slab = (kalloc_slab_t *)mem;
	ASSERT_PANIC(slab_ret == slab, "Slab ret is not at slab");

	max_num = slab->max_num;

	for (unsigned int i = 0; i < max_num; i++) {
		vals[i] = rand_prng();
	}

	for (unsigned int i = 0; i < max_num; i++) {
		ptrs[i] = kalloc_slab_alloc(slab);
		*ptrs[i] = vals[i];
	}

	alloc = 0;
	free = 0;
	for (unsigned int i = 0; i < max_num; i++) {
		//DEBUG_DATA_DIGIT("i=", i);
		rand = rand_prng() % SLAB_TEST_ALLOC_WEIGHT;
		ASSERT_PANIC(_kalloc_slab_verify(slab, i, vals[i]) == 0, "Val not expected");
		ret = kalloc_slab_free(slab, ptrs[i]);
		free ++;

		if (rand < SLAB_TEST_ALLOC_WEIGHT && free > alloc) {
			ptrs[alloc] = kalloc_slab_alloc(slab);
			*ptrs[alloc] = vals[alloc];
			ASSERT_PANIC(_kalloc_slab_verify(slab, alloc, vals[alloc]) == 0, "Val not expected");
			alloc++;
		}
	}

	for (int i = free; i < alloc; i++) {
		ASSERT_PANIC(_kalloc_slab_verify(slab, i, vals[i]) == 0, "Val not expected");
		ret = kalloc_slab_free(slab, ptrs[i]);
		ASSERT_PANIC(!ret, "slab free failed");
	}

	//mem
	mm_earlypage_shrink(SLAB_TEST_NUM_PAGES);
	//ptrs
	mm_earlypage_shrink(SLAB_TEST_DATA_PAGES);
	//vals
	mm_earlypage_shrink(SLAB_TEST_DATA_PAGES);
	//slab
	mm_earlypage_shrink(1);

	DEBUG("--- Slab test end---");
}

void _kalloc_verify_cache_obj(uint64_t * obj_addr, uint64_t expected)
{	
	int fail = 0;
	if (*obj_addr != expected)  {
		fail = 1;
	}
	/*
	DEBUG_FUNC("Checking obj addr=", obj_addr);
	DEBUG_FUNC("Expected=", expected);
	DEBUG_FUNC("Found=", *obj_addr);
	*/
	ASSERT_PANIC(!fail, "Obj addr is not expected");
}

void kalloc_cache_test()
{
	DEBUG("----Cache test start-----");

	#define CACHE_TEST_FREE_RATE 3
	#define CACHE_TEST_ALLOC_RATE (5 + CACHE_TEST_FREE_RATE)
	#define CACHE_TEST_PAGE_NUM 2
	#define CACHE_TEST_NUM_SLABS 5
	#define CACHE_TEST_DATA_PAGES (CACHE_TEST_NUM_SLABS * CACHE_TEST_PAGE_NUM)
	#define CACHE_TEST_OBJ_TYPE uint64_t
	#define CACHE_TEST_OBJ_SIZE sizeof(CACHE_TEST_OBJ_TYPE)
	//#define CACHE_TEST_SLAB_PTR_PAGES ((CACHE_TEST_NUM_SLABS * sizeof(kalloc_slab_t *) / PAGE_SIZE)  + 1)
	#define CACHE_TEST_SLAB_PTR_PAGES CACHE_TEST_NUM_SLABS
	//#define CACHE_TEST_VALS_BUF_NUM (CACHE_TEST_NUM_SLABS * PAGE_SIZE) / sizeof(uint64_t)
	#define CACHE_TEST_VALS_BUF_NUM CACHE_TEST_NUM_SLABS
	//#define CACHE_TEST_SLAB_OBJ_MAX_NUM (CACHE_TEST_NUM_SLABS * PAGE_SIZE) / sizeof(uint64_t *)
	#define CACHE_TEST_SLAB_OBJ_MAX_NUM CACHE_TEST_NUM_SLABS

	int ret = 0;
	kalloc_cache_t cache;
	kalloc_slab_t * slab;
	unsigned int max_alloc_num = 0;
	
	uint64_t * vals = (uint64_t *)mm_earlypage_alloc(CACHE_TEST_DATA_PAGES);
	memset(vals, 0, CACHE_TEST_DATA_PAGES * PAGE_SIZE);
	for (int i = 0; i < CACHE_TEST_VALS_BUF_NUM; i++) {
		((uint32_t*)&vals[i])[0] = rand_prng();
		((uint32_t*)&vals[i])[1] = rand_prng();
	}

	kalloc_slab_t ** slab_ptrs = (kalloc_slab_t **)mm_earlypage_alloc(CACHE_TEST_DATA_PAGES);
	memset(slab_ptrs, 0, CACHE_TEST_DATA_PAGES * PAGE_SIZE);
	
	CACHE_TEST_OBJ_TYPE ** obj_ptrs = (CACHE_TEST_OBJ_TYPE **)mm_earlypage_alloc(CACHE_TEST_DATA_PAGES);
	memset(obj_ptrs, 0, CACHE_TEST_DATA_PAGES * PAGE_SIZE);

	ret = kalloc_cache_init(&cache, CACHE_TEST_OBJ_SIZE, CACHE_TEST_PAGE_NUM, NULL,
							NULL, KALLOC_CACHE_NO_EXPAND_F | KALLOC_CACHE_NO_SHRINK_F);
	ASSERT_PANIC(!ret, "Kalloc cache init failed");

	for (int i = 0; i < CACHE_TEST_NUM_SLABS; i++) {
		uint8_t * page_mem = mm_earlypage_alloc(CACHE_TEST_PAGE_NUM);
		ASSERT_PANIC(page_mem, "mm alloc failed");

		slab = kalloc_cache_add_slab_pages(&cache, page_mem, CACHE_TEST_PAGE_NUM);
		ASSERT_PANIC(slab, "Kalloc cache add slab page failed");
		slab_ptrs[i] = slab;
	}

	max_alloc_num = cache.max_num;

	for (unsigned int i = 0; i < max_alloc_num; i++) {
		obj_ptrs[i] = (CACHE_TEST_OBJ_TYPE *)kalloc_cache_alloc(&cache);
		//DEBUG_FUNC("Obj alloced at addr=", (uint64_t)obj_ptrs[i]);
		ASSERT_PANIC(obj_ptrs[i], "Obj ptr is invalid, kalloc cache alloc failed");
		*obj_ptrs[i] = vals[i];
	}

	for (unsigned int i = 0; i < max_alloc_num; i++) {
		//DEBUG_FUNC("cache free from addr=", (uint64_t)obj_ptrs[i]);
		_kalloc_verify_cache_obj(obj_ptrs[i], vals[i]);
		ret = kalloc_cache_free(&cache, obj_ptrs[i]);
		ASSERT_PANIC(!ret, "Cache free failed");
	}

	ASSERT_PANIC(ll_list_size(&cache.free_list) == CACHE_TEST_NUM_SLABS, "Free list does not match free slabs");
	ASSERT_PANIC(ll_list_size(&cache.partial_list) == 0, "partial list does not match free slabs");
	ASSERT_PANIC(ll_list_size(&cache.full_list) == 0, "full list does not match free slabs");
	ASSERT_PANIC(cache.num == 0, "Cache is not empty after full alloc/free");

	DEBUG("----Linear alloc/free Cache test end-----");
	DEBUG("---- Rand Cache test start-----");
	memset(obj_ptrs, 0, CACHE_TEST_DATA_PAGES * PAGE_SIZE);

	unsigned int free = 0;
	unsigned int alloc = 0;
	for (unsigned int i = 0; i < max_alloc_num; i++) {
		int rand = rand_prng() % (CACHE_TEST_ALLOC_RATE + 1);

		if (rand <= CACHE_TEST_FREE_RATE && free < alloc) {
			unsigned int rand_index = rand_prng() % (alloc + 1);

			if (!obj_ptrs[rand_index])
				continue;
			
			_kalloc_verify_cache_obj(obj_ptrs[rand_index], vals[rand_index]);
			ret = kalloc_cache_free(&cache, obj_ptrs[rand_index]);
			ASSERT_PANIC(!ret, "Cache free failed");
			//DEBUG_FUNC("cache free from addr=", (uint64_t)obj_ptrs[rand_index]);

			obj_ptrs[rand_index] = NULL;
			free++;

		} else {
			obj_ptrs[i] = (CACHE_TEST_OBJ_TYPE *)kalloc_cache_alloc(&cache);
			DEBUG_FUNC("Obj alloced at addr=", (uint64_t)obj_ptrs[i]);
			ASSERT_PANIC(obj_ptrs[i], "Obj ptr is invalid, kalloc cache alloc failed");
			*obj_ptrs[i] = vals[i];
			alloc++;
		}
	}

	// Free all valid allocs
	for (int i = 0; i < alloc; i++) {
		if (!obj_ptrs[i])
			continue;
		
		_kalloc_verify_cache_obj(obj_ptrs[i], vals[i]);
		ret = kalloc_cache_free(&cache, obj_ptrs[i]);

		ASSERT_PANIC(!ret, "cache free failed");
	}

	//vals
	mm_earlypage_shrink(CACHE_TEST_DATA_PAGES);
	//slab_ptrs
	mm_earlypage_shrink(CACHE_TEST_DATA_PAGES);
	//obj_ptrs
	mm_earlypage_shrink(CACHE_TEST_DATA_PAGES);
	//slab pages
	for (int i = 0; i < CACHE_TEST_NUM_SLABS; i++) {
		mm_earlypage_shrink(CACHE_TEST_PAGE_NUM);
	}


	DEBUG("----Cache test end-----");
}

void _validate_and_set_mm_alloc(uint64_t addr, unsigned int memorder, uint32_t set_val)
{
	unsigned int page_index_start = addr / PAGE_SIZE;
	unsigned int page_num = 1 << memorder;
	mm_global_area_t * global_area = mm_global_area();
	mm_page_t * pages = global_area->global_pages;

	//DEBUG_DATA_DIGIT("validate page index=", page_index_start);
	for (int i = 0; i < page_num; i++) {
		//DEBUG_DATA_DIGIT("val page index=", page_index_start + i);
		ASSERT_PANIC(mm_page_is_valid(page_index_start + i), "Page is not valid)");
	}

	uint64_t end_addr = addr + (page_num * PAGE_SIZE);

	for (uint64_t curr_addr = addr; curr_addr < end_addr; curr_addr += sizeof(uint32_t)) {
		*((uint32_t *)curr_addr) = set_val; 
	}
}

#define MM_TEST_DATA_TYPE uint32_t

void _validate_mm_free(uint64_t addr, unsigned int memorder, MM_TEST_DATA_TYPE expected_val)
{
	unsigned int page_index_start = addr / PAGE_SIZE;
	unsigned int page_num = 1 << memorder;
	mm_global_area_t * global_area = mm_global_area();
	mm_page_t * pages = global_area->global_pages;

	DEBUG_DATA_DIGIT("---Checking memorder=", memorder);
	DEBUG_DATA_DIGIT("---Checking page_num=", page_num);
	for (int i = 0; i < page_num; i++) {
		ASSERT_PANIC(!mm_page_is_valid(page_index_start + i), "Page is valid)");
	}

	uint64_t end_addr = addr + (page_num * PAGE_SIZE);

	for (uint64_t curr_addr = addr; curr_addr < end_addr; curr_addr += sizeof(MM_TEST_DATA_TYPE)) {
		if (*((uint32_t *)curr_addr) != expected_val) {
			DEBUG_PANIC("MM FREE IS NOT EXPECTED VALUE");
		}
	}
}

/* run test in debug only as we will be writing directly to memory and reserving mem. */
void mm_test()
{	
	DEBUG("--- MM TEST START ----");

	#define MM_TEST_FREE_WEIGHT 3
	#define MM_TEST_ALLOC_WEIGHT (4 + MM_TEST_FREE_WEIGHT)
	#define MM_TEST_MEMORDER_RANGE 8

	#define MM_TEST_DATA_PAGES 16
	#define MM_TEST_NUM 1024  * 2 * 2 * 2
	#define MM_TEST_NULL_ADDR ((uint64_t)~0)

	uint64_t found_addr;
	kalloc_buddy_t * buddy;
	uint64_t reserve_addr = 0;
	int ret = 0;
	unsigned int free = 0;
	unsigned int alloc = 0;

	uint8_t ** ptrs = (uint8_t **)mm_earlypage_alloc(MM_TEST_DATA_PAGES);
	MM_TEST_DATA_TYPE * vals = (MM_TEST_DATA_TYPE *)mm_earlypage_alloc(MM_TEST_DATA_PAGES);
	unsigned int * memorders = (unsigned int)mm_earlypage_alloc(MM_TEST_DATA_PAGES);

	uint64_t free_reserve_addr = MM_TEST_NULL_ADDR;
	uint64_t free_reserve_memorder = 0;

	unsigned int val_num = (MM_TEST_DATA_PAGES * PAGE_SIZE) / sizeof(MM_TEST_DATA_TYPE);
	for (int i = 0; i < val_num; i++)  {
		vals[i] = rand_prng();
	}

	for (int i = 0; i < MM_TEST_NUM; i++) {
		int rand_op = rand_prng() % (MM_TEST_ALLOC_WEIGHT);
		int rand_memorder = rand_prng() % (MM_TEST_MEMORDER_RANGE + 1);
		//int rand_memorder = 1;

		if (rand_op < MM_TEST_FREE_WEIGHT && alloc > free) {
			DEBUG("MM TEST FREE");
			
			int rand_free = rand_prng() % (alloc + 1);
			//continue;
			if (!ptrs[rand_free])
				continue;

			buddy = kalloc_get_buddy_from_addr(ptrs[rand_free]);
			ASSERT_PANIC(buddy->buddy_memorder == memorders[rand_free], "Buddy is not the past memorder");

			ret = kalloc_page_free_pages(ptrs[rand_free], 0);
			ASSERT_PANIC(!ret, "mm_free_pages failed");
			_validate_mm_free((uint64_t)ptrs[rand_free], memorders[rand_free], vals[rand_free]);
			
			if ((uint64_t)ptrs[rand_free] < free_reserve_addr) {
				free_reserve_addr = (uint64_t)ptrs[rand_free];
				free_reserve_memorder = memorders[rand_free];
				DEBUG("FREE RESERVE");
			}

			DEBUG_FUNC("MM Free at addr=", ptrs[rand_free]);
			//DEBUG_FUNC("TO=", (uint64_t)ptrs[rand_free] + MM_MEMORDER_TO_PAGES(memorders[rand_free]) * PAGE_SIZE);
			DEBUG_FUNC("MM free at memorder=", memorders[rand_free]);

			ptrs[rand_free] = NULL;

			free++;
		} else {
			int alloc_op = rand_prng() % 2;

			if (alloc_op) {
				DEBUG("MM TEST ALLOC");
				DEBUG_FUNC("MM alloc at memorder=", rand_memorder);
				ptrs[alloc] = kalloc_page_alloc_pages(rand_memorder, 0);
				ASSERT_PANIC(ptrs[alloc], "MM alloc pages failed");
				memorders[alloc] = rand_memorder;

				if (free_reserve_addr != MM_TEST_NULL_ADDR && 
					!RANGES_OVERLAP_INCLUSIVE(free_reserve_addr, MM_MEMORDER_TO_PAGES(free_reserve_memorder) * PAGE_SIZE, 
									(uint64_t)ptrs[alloc], MM_MEMORDER_TO_PAGES(rand_memorder) * PAGE_SIZE)) {
					free_reserve_addr = MM_TEST_NULL_ADDR;
					DEBUG("ALLOC RESERVE");
				}

				DEBUG_FUNC("ALLOC at addr=", ptrs[alloc]);
				DEBUG_FUNC_DIGIT("ALLOC memorder=", rand_memorder);

				_validate_and_set_mm_alloc((uint64_t)ptrs[alloc], rand_memorder, vals[alloc]);
			} else {			
				DEBUG("MM RESERVE");
				if (free_reserve_addr == MM_TEST_NULL_ADDR)
					continue;

				DEBUG_FUNC("MM reserve at addr=", free_reserve_addr);
				DEBUG_FUNC("MM reserve at memorder=", free_reserve_memorder);
				
				ret = kalloc_page_reserve_pages(free_reserve_addr, free_reserve_memorder, 0);
				ASSERT_PANIC(!ret, "MM reserve failed");

				ptrs[alloc] = (MM_TEST_DATA_TYPE*)free_reserve_addr;
				memorders[alloc] = free_reserve_memorder;

				free_reserve_addr = MM_TEST_NULL_ADDR;

				_validate_and_set_mm_alloc((uint64_t)ptrs[alloc], free_reserve_memorder, vals[alloc]);
			}

			alloc++;
		}
	}

	DEBUG("-MM test done, freeing remaining");

	for (unsigned int i = 0; i < alloc; i++) {
		if (!ptrs[i])
			continue;
		buddy = kalloc_get_buddy_from_addr(ptrs[i]);
		ASSERT_PANIC(buddy->buddy_memorder == memorders[i], "Buddy is not the past memorder");
		ret = kalloc_page_free_pages(ptrs[i], 0);
		ASSERT_PANIC(!ret, "mm_free_pages failed");
		_validate_mm_free((uint64_t)ptrs[i], memorders[i], vals[i]);
	}

	for (int i = MM_RESERVE_AREA_INDEX; i < mm_global_area()->area_count; i++) {
		if (mm_global_area()->global_areas[i].free_page_num != (MM_AREA_SIZE / PAGE_SIZE)){
			DEBUG_DATA_DIGIT("Curr_area pages=",mm_global_area()->global_areas[i].free_page_num);
			DEBUG_DATA_DIGIT("Total pages=",(MM_AREA_SIZE / PAGE_SIZE));
			DEBUG_PANIC("Free page num is not total freed");
		}

		if (ll_list_size(&mm_global_area()->global_areas[i].free_buddy_list[MM_MAX_ORDER]) != 2) {
			DEBUG_PANIC("MM MAX ORDER free nodes are not 2 after completly freeing");
		}
	}

	DEBUG("--- MM TEST DONE ----");
}

/*
static void mmu_test()
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
*/

void _set_kalloc_alloc(void * mem_start, size_t size, uint32_t val)
{
    uint64_t end_addr = (uint64_t)mem_start + size;

	for (uint64_t curr_addr = (uint64_t)mem_start; curr_addr < end_addr; curr_addr += sizeof(uint32_t)) {
		*((uint32_t *)curr_addr) = val; 
	}
}

void _validate_kalloc_alloc(void * mem_start, size_t size, uint32_t val) {
    uint64_t end_addr = (uint64_t)mem_start + size;

	for (uint64_t curr_addr = (uint64_t)mem_start; curr_addr < end_addr; curr_addr += sizeof(uint32_t)) {
        ASSERT_PANIC(*((uint32_t *)curr_addr) == val, "Kalloc validate is not given val");
	}
}

void kalloc_test()
{
    DEBUG("--- Kalloc test start ---");

    #define KALLOC_TEST_NUM 512
    #define KALLOC_TEST_DATA_PAGES (((KALLOC_TEST_NUM * sizeof(void*))/PAGE_SIZE) + 1)
    #define KALLOC_TEST_PAGE_NUM_MAX 64
    #define KALLOC_TEST_ALLOC_SIZE_MAX 1024

    void ** ptrs = (void **)mm_earlypage_alloc(KALLOC_TEST_DATA_PAGES);
    size_t * sizes = (size_t *)mm_earlypage_alloc(KALLOC_TEST_DATA_PAGES);
	uint32_t * vals = (uint32_t *)mm_earlypage_alloc(KALLOC_TEST_DATA_PAGES);
    int ret = 0;

    memset(ptrs, 0, KALLOC_TEST_DATA_PAGES * PAGE_SIZE);
    memset(vals, 0, KALLOC_TEST_DATA_PAGES * PAGE_SIZE);
    memset(sizes, 0, KALLOC_TEST_DATA_PAGES * PAGE_SIZE);

    DEBUG_DATA_DIGIT("Starting test with data page size=", KALLOC_TEST_DATA_PAGES);

    for (int i = 0; i < KALLOC_TEST_NUM; i++) {
        vals[i] = rand_prng();
    }

    for (int i = 0; i < KALLOC_TEST_NUM; i++) {

        int op = rand_prng() % 2;

        if (op == 1) {
            unsigned int page_num = rand_prng() % KALLOC_TEST_PAGE_NUM_MAX;

            DEBUG_DATA_DIGIT("Alloc with pagenum=", page_num);
            ptrs[i] = kalloc_alloc(page_num * PAGE_SIZE, 0);
            ASSERT_PANIC(ptrs[i], "Kalloc alloc failed.");
            sizes[i] = page_num * PAGE_SIZE;
        } else {
            size_t size = rand_prng() % KALLOC_TEST_ALLOC_SIZE_MAX;
            size = ALIGN_UP(size, sizeof(uint32_t));

            DEBUG_DATA_DIGIT("Alloc with size=", size);
            ptrs[i] = kalloc_alloc(size, 0);
            ASSERT_PANIC(ptrs[i], "Kalloc alloc failed.");
            sizes[i] = size;
        }

        _set_kalloc_alloc(ptrs[i], sizes[i], vals[i]);
    }

    for (int i = 0; i  < KALLOC_TEST_NUM; i++) {
        _validate_kalloc_alloc(ptrs[i], sizes[i], vals[i]);

        DEBUG_DATA_DIGIT("freeing size=", sizes[i]);
        DEBUG_DATA("Freeing obj=", ptrs[i]);

        ret = kalloc_free(ptrs[i], 0);
        ASSERT_PANIC(!ret, "Kalloc free failed.");
    }


    DEBUG("--- Kalloc test end---");
}
