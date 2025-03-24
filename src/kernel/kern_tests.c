
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
#include <kernel/early_mm.h>
#include <common/math.h>
#include <common/rand.h>

#define LL_TEST_NUM 6

static void ll_info_add(ll_head_t * head, ll_node_t * nodes, void * data, unsigned int index)
{
	int ret = 0;
	ll_node_init(&nodes[index], data, head->type);
	ret = ll_push_list(head, &nodes[index]);
	ASSERT_PANIC(!ret, "Ll push list failed");
}

static void list_test()
{
	DEBUG("---LIST LL TEST ---");
	ll_node_t * p;
	ll_head_t head;
	ll_node_t nodes[LL_TEST_NUM];
	list_node_t node1;
	list_node_t node2;
	unsigned int i = 0;

	memset(&nodes[0], 0, sizeof(list_node_t) * LL_TEST_NUM);

	ll_head_init(&head, LIST_NODE_T);

	for (unsigned int i = 0; i < LL_TEST_NUM; i++) {
		ll_info_add(&head, &nodes[0], NULL, i);
		/*
		for (unsigned int j = 0; j < LL_TEST_NUM; j++) {
			if (nodes[j].list.next) {
				DEBUG_DATA("next=", nodes[j].list.next);
			} else
				break;
		}*/
	}

	ll_push_list(&head, (ll_node_t *)&node1);
	ll_push_list(&head, (ll_node_t *)&node2);

	ASSERT_PANIC((list_node_t *)node1.next == &node2, "Inorder push failed");
	ll_delete_node(&head, (ll_node_t *)&node1);
	ASSERT_PANIC(head.last == (ll_node_t *)&node2, "Head is not pointing at last");
	ll_delete_node(&head, (ll_node_t *)&node2);
	ASSERT_PANIC(head.last == &nodes[LL_TEST_NUM - 1], "Head is not pointing at last");
	ASSERT_PANIC(head.next == &nodes[0], "Head is not pointing at first");

	i = 0;
	LL_ITER_LIST(&head, p) {
		ASSERT_PANIC(head.next == &nodes[i], "Inorder delete failed");
		ll_delete_node(&head, &nodes[i]);
		i ++;
	}

	ASSERT_PANIC(head.count == 0, "Head count is not zero");
	DEBUG("Delete done");

	/*
	i = 0;
	LL_ITER_LIST(&head, p) {
		DEBUG_DATA_DIGIT("list i = ", i);
		DEBUG_DATA("ptr=", p);
		DEBUG_DATA("next", p->list.next);
		i ++;
	} */

	DEBUG("---LIST LL DONE ---");
}

void dll_test()
{
	DEBUG("---DLL LIST TEST ---");

	ll_node_t * p;
	ll_head_t head;
	ll_node_t nodes[LL_TEST_NUM];
	ll_node_t node1;
	ll_node_t node2;
	unsigned int i = 0;

	memset(&nodes[0], 0, sizeof(list_node_t) * LL_TEST_NUM);

	ll_head_init(&head, DLL_NODE_T);

	for (unsigned int i = 0; i < LL_TEST_NUM; i++) {
		ll_info_add(&head, &nodes[0], (void*)i, i);

		/*
		for (unsigned int j = 0; j < LL_TEST_NUM; j++) {
			if (nodes[j].list.next) {
				DEBUG_DATA("next=", nodes[j].list.next);
				DEBUG_DATA("data=", nodes[j].sll.data);
				DEBUG_DATA("last=", nodes[j].dll.last);
			} else
				break;
		} */
	}

	ASSERT_PANIC(head.next == &nodes[0], "Head is not pointing at first4");

	ll_push_list(&head, &node1);
	ll_push_list(&head, &node2);

	//DEBUG_DATA("head next=", head.next);

	ASSERT_PANIC(head.next == &nodes[0], "Head is not pointing at first1");
	ASSERT_PANIC((ll_node_t *)node1.dll.next == &node2, "Inorder push failed");
	ASSERT_PANIC((ll_node_t *)node2.dll.last == &node1, "Inorder push failed");
	ll_delete_node(&head, &node1);
	ASSERT_PANIC((ll_node_t *)head.next == &nodes[0], "Head is not pointing at first2");
	ASSERT_PANIC(head.last == &node2, "Head is not pointing at last");
	ASSERT_PANIC((ll_node_t *)node2.dll.last == &nodes[LL_TEST_NUM - 1], "Inorder push failed");
	ASSERT_PANIC((ll_node_t *)nodes[LL_TEST_NUM - 1].dll.next == &node2, "Inorder push failed");
	ll_delete_node(&head, &node2);
	ASSERT_PANIC(head.next == &nodes[0], "Head is not pointing at first3");
	ASSERT_PANIC(head.last == &nodes[LL_TEST_NUM - 1], "Head is not pointing at last");
	ASSERT_PANIC(head.next == &nodes[0], "Head is not pointing at first");

	//DEBUG_DATA("head next=", head.next);
	i = 0;
	LL_ITER_LIST(&head, p) {
		ASSERT_PANIC(head.next == &nodes[i], "Inorder delete failed");
		ll_delete_node(&head, &nodes[i]);
		i ++;
	}

	ASSERT_PANIC(head.count == 0, "Head count is not zero");
	//DEBUG("Delete done");

	/*
	i = 0;
	LL_ITER_LIST(&head, p) {
		DEBUG_DATA_DIGIT("list i = ", i);
		DEBUG_DATA("ptr=", p);
		DEBUG_DATA("next", p->list.next);
		i ++;
	} */
}

void ll_test()
{
	list_test();
	dll_test();
}

#define QUEUE_CHAIN_NUM 20
void queue_test()
{
	queue_t qe;
	queue_t qe_prev;
	queue_head_t qhead;
	queue_chain_t qes[QUEUE_CHAIN_NUM];
	queue_init(&qhead);
	queue_t * qe_p;

	for (int i = 0; i < QUEUE_CHAIN_NUM; i++) {
		enqueue_tail(&qhead, &qes[i]);
	}

	queue_t qe_iter;
	int iter = 0;
	queue_iter_safe(&qhead, qe, qe_prev) {
		
		DEBUG_DATA_DIGIT("I=", iter);
		qe_iter = dequeue_head(&qhead);
		
		if (&qes[iter] != qe_iter) {
			DEBUG_PANIC("Wrong element 1");
		}
		
		if (&qes[iter] != qe) {
			DEBUG_PANIC("Wrong element 2");
		}

		if (qes[iter].next->prev == qe) {
			DEBUG_PANIC("Failed dequeue");
		}

		if (qe_prev != &qhead) {
			DEBUG_PANIC("Qe prev is not head");
		}

		queue_zero(qe);
		iter++;
	}

	DEBUG("Queue test success");
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

int _kalloc_slab_verify(kalloc_slab_t * slab, unsigned int index, uint32_t expected)
{
	uint32_t * ptr = (uint32_t*)((uint8_t *)slab->mem_ptr + slab->obj_size * index);
	uint32_t found = *ptr;
	DEBUG_FUNC("Addr checked at addr=", (uint64_t)ptr);
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

	ASSERT_PANIC(mem && ptrs && vals && slab, "Kalloc slab mem alloc failed");

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
	ret = mm_earlypage_shrink(SLAB_TEST_NUM_PAGES);
	ASSERT_PANIC(!ret, "page free failed");
	//ptrs
	ret = mm_earlypage_shrink(SLAB_TEST_DATA_PAGES);
	ASSERT_PANIC(!ret, "page free failed");
	//vals
	ret = mm_earlypage_shrink(SLAB_TEST_DATA_PAGES);
	ASSERT_PANIC(!ret, "page free failed");
	//slab
	ret = mm_earlypage_shrink(1);
	ASSERT_PANIC(!ret, "page free failed");

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
	kalloc_slab_t ** slab_ptrs = (kalloc_slab_t **)mm_earlypage_alloc(CACHE_TEST_DATA_PAGES);
	CACHE_TEST_OBJ_TYPE ** obj_ptrs = (CACHE_TEST_OBJ_TYPE **)mm_earlypage_alloc(CACHE_TEST_DATA_PAGES);

	ASSERT_PANIC(vals && slab_ptrs && obj_ptrs, "Earlypage alloc failed");

	memset(vals, 0, CACHE_TEST_DATA_PAGES * PAGE_SIZE);
	memset(slab_ptrs, 0, CACHE_TEST_DATA_PAGES * PAGE_SIZE);
	memset(obj_ptrs, 0, CACHE_TEST_DATA_PAGES * PAGE_SIZE);

	for (int i = 0; i < CACHE_TEST_VALS_BUF_NUM; i++) {
		((uint32_t*)&vals[i])[0] = rand_prng();
		((uint32_t*)&vals[i])[1] = rand_prng();
	}

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
	ret = mm_earlypage_shrink(CACHE_TEST_DATA_PAGES);
	ASSERT_PANIC(!ret, "Earlypage free failed");
	//slab_ptrs
	ret = mm_earlypage_shrink(CACHE_TEST_DATA_PAGES);
	ASSERT_PANIC(!ret, "Earlypage free failed");
	//obj_ptrs
	ret = mm_earlypage_shrink(CACHE_TEST_DATA_PAGES);
	ASSERT_PANIC(!ret, "Earlypage free failed");
	//slab pages
	for (int i = 0; i < CACHE_TEST_NUM_SLABS; i++) {
		ret = mm_earlypage_shrink(CACHE_TEST_PAGE_NUM);
		ASSERT_PANIC(!ret, "Earlypage free failed");
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
	unsigned int area_count;
	uint64_t reserve_addr = 0;
	int ret = 0;
	unsigned int free = 0;
	unsigned int alloc = 0;

	uint8_t ** ptrs = (uint8_t **)mm_earlypage_alloc(MM_TEST_DATA_PAGES);
	MM_TEST_DATA_TYPE * vals = (MM_TEST_DATA_TYPE *)mm_earlypage_alloc(MM_TEST_DATA_PAGES);
	unsigned int * memorders = (unsigned int *)mm_earlypage_alloc(MM_TEST_DATA_PAGES);
	unsigned int * area_page_count = (unsigned int *)mm_earlypage_alloc(2);

	ASSERT_PANIC(ptrs && vals && memorders, "Earlypage alloc failed");

	memset(ptrs, 0, MM_TEST_DATA_PAGES * PAGE_SIZE);
	memset(vals, 0, MM_TEST_DATA_PAGES * PAGE_SIZE);
	memset(memorders, 0, MM_TEST_DATA_PAGES * PAGE_SIZE);

	// Record the current page count to make sure we return to the same amount
	// after freeing
	area_count = mm_global_area()->area_count;
	for (int i = 0; i < area_count; i++) {
		area_page_count[i] = mm_global_area()->global_areas[i].free_page_num;
	}

	uint64_t free_reserve_addr = MM_TEST_NULL_ADDR;
	uint64_t free_reserve_memorder = 0;

	unsigned int val_num = (MM_TEST_DATA_PAGES * PAGE_SIZE) / sizeof(MM_TEST_DATA_TYPE);
	for (int i = 0; i < val_num; i++)  {
		vals[i] = rand_prng();
	}

	for (int i = 0; i < MM_TEST_NUM; i++) {
		int rand_op = rand_prng() % (MM_TEST_ALLOC_WEIGHT);
		int rand_memorder = rand_prng() % (MM_TEST_MEMORDER_RANGE + 1);
		//int rand_memorder = 0;

		if (rand_op < MM_TEST_FREE_WEIGHT && alloc > free) {
			DEBUG("MM TEST FREE");

			int rand_free = rand_prng() % (alloc + 1);
			//continue;
			if (!ptrs[rand_free])
				continue;

			DEBUG_FUNC("MM Free at addr=", (uint64_t)ptrs[rand_free]);
			DEBUG_FUNC("MM free at memorder=", memorders[rand_free]);

			buddy = kalloc_get_buddy_from_addr((uint64_t)ptrs[rand_free]);
			ASSERT_PANIC(buddy->buddy_memorder == memorders[rand_free], "Buddy is not the past memorder");

			ret = kalloc_page_free_pages((uint64_t)ptrs[rand_free], 0);
			ASSERT_PANIC(!ret, "mm_free_pages failed");
			_validate_mm_free((uint64_t)ptrs[rand_free], memorders[rand_free], vals[rand_free]);

			if ((uint64_t)ptrs[rand_free] < free_reserve_addr) {
				free_reserve_addr = (uint64_t)ptrs[rand_free];
				free_reserve_memorder = memorders[rand_free];
				DEBUG("FREE RESERVE");
			}

			DEBUG_DATA("Free done at=", (uint64_t)ptrs[rand_free]);

			ptrs[rand_free] = NULL;

			free++;
		} else {
			int alloc_op = rand_prng() % 2;

			if (alloc_op) {
				DEBUG("MM TEST ALLOC");
				DEBUG_FUNC("MM alloc at memorder=", rand_memorder);
				ptrs[alloc] = (uint8_t *)kalloc_page_alloc_pages(rand_memorder, 0);
				ASSERT_PANIC(ptrs[alloc], "MM alloc pages failed");
				memorders[alloc] = rand_memorder;

				if (free_reserve_addr != MM_TEST_NULL_ADDR &&
					!RANGES_OVERLAP_INCLUSIVE(free_reserve_addr, MM_MEMORDER_TO_PAGES(free_reserve_memorder) * PAGE_SIZE, 
									(uint64_t)ptrs[alloc], MM_MEMORDER_TO_PAGES(rand_memorder) * PAGE_SIZE)) {
					free_reserve_addr = MM_TEST_NULL_ADDR;
					DEBUG("ALLOC RESERVE");
				}

				DEBUG_FUNC("ALLOC at addr=", (uint64_t)ptrs[alloc]);
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

				ptrs[alloc] = (uint8_t *)free_reserve_addr;
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
		buddy = kalloc_get_buddy_from_addr((uint64_t)ptrs[i]);
		ASSERT_PANIC(buddy->buddy_memorder == memorders[i], "Buddy is not the past memorder");
		ret = kalloc_page_free_pages((uint64_t)ptrs[i], 0);
		ASSERT_PANIC(!ret, "mm_free_pages failed");
		_validate_mm_free((uint64_t)ptrs[i], memorders[i], vals[i]);
	}


	for (int i = 0; i < area_count; i++) {
		if (mm_global_area()->global_areas[i].free_page_num != area_page_count[i]) {
			DEBUG_PANIC("Free page num is not the same as started");
		}
	}

	//ptrs
	ret = mm_earlypage_shrink(MM_TEST_DATA_PAGES);
	ASSERT_PANIC(!ret, "Earlypage free failed");
	//vals
	ret = mm_earlypage_shrink(MM_TEST_DATA_PAGES);
	ASSERT_PANIC(!ret, "Earlypage free failed");
	//memorders
	ret = mm_earlypage_shrink(MM_TEST_DATA_PAGES);
	ASSERT_PANIC(!ret, "Earlypage free failed");

	DEBUG("--- MM TEST DONE ----");
}

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
        DEBUG_DATA("Freeing obj=", (uint64_t)ptrs[i]);

        ret = kalloc_free(ptrs[i], 0);
        ASSERT_PANIC(!ret, "Kalloc free failed.");
    }


    DEBUG("--- Kalloc test end---");
}
