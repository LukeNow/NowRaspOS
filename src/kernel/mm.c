#include <stdint.h>
#include <stddef.h>
#include <common/assert.h>
#include <kernel/addr_defs.h>
#include <kernel/uart.h>
#include <common/lock.h>
#include <kernel/mm.h>
#include <kernel/kalloc_cache.h>
#include <kernel/kalloc_page.h>
#include <kernel/early_mm.h>
#include <common/math.h>
#include <kernel/mmu.h>
#include <common/linkedlist.h>

DEFINE_SPINLOCK(mm_lock);

static mm_global_area_t global_area;

static int mm_initialized = 0;

/* Early heap wrappers. For now since we are 1-1 mapping memory we can just
 * add the virtual offset to indicate we are in kernel memory. */
static void align_mem(size_t size)
{
    align_early_mem(size);
}

static uint8_t * data_alloc(size_t size)
{
    return early_data_alloc(size) + MMU_UPPER_ADDRESS;
}

static uint8_t * page_data_alloc(unsigned page_num)
{
    return early_page_data_alloc(page_num) + MMU_UPPER_ADDRESS;
}

int mm_is_initialized()
{
    return mm_initialized;
}

mm_global_area_t * mm_global_area()
{
    return &global_area;
}

int mm_page_is_valid(unsigned int page_index)
{
    return mm_global_area()->global_pages[page_index].page_flags & MM_PAGE_VALID;
}

int mm_pages_are_valid(unsigned int start_page_index, unsigned int page_num)
{
    for (int i = 0; i < page_num; i++) {
        if (!mm_page_is_valid(start_page_index))
            return 0;
        start_page_index++;
    }

    return 1;
}

void mm_mark_page_valid(unsigned int page_index)
{
    ASSERT_PANIC(!mm_page_is_valid(page_index), "MM double marking page valid.");
    mm_global_area()->global_pages[page_index].page_flags |= MM_PAGE_VALID;
}

void mm_mark_pages_valid(unsigned int start_page_index, unsigned int page_num)
{
    for (int i = 0; i < page_num; i++) {
        mm_mark_page_valid(start_page_index);
        start_page_index++;
    }
}

void mm_mark_page_free(unsigned int page_index)
{
    ASSERT_PANIC(mm_page_is_valid(page_index), "MM double marking page free.");
    mm_global_area()->global_pages[page_index].page_flags &= ~MM_PAGE_VALID;
}

void mm_mark_pages_free(unsigned int start_page_index, unsigned int page_num)
{
    for (int i = 0; i < page_num; i++) {
        mm_mark_page_free(start_page_index);
        start_page_index++;
    }
}

int mm_pages_are_free(unsigned int start_page_index, unsigned int page_num)
{
    for (int i = 0; i < page_num; i++) {
        if (mm_page_is_valid(start_page_index))
            return 0;
        start_page_index++;
    }

    return 1;
}

unsigned int mm_pages_to_memorder(unsigned int page_num)
{
    page_num = math_align_power2_64(page_num);
    return math_log2_64(page_num);
}

unsigned int mm_memorder_to_pages(unsigned int memorder)
{
    return 1 << memorder;
}

void * mm_get_page_obj_ptr(unsigned int page_index)
{
    return mm_global_area()->global_pages[page_index].obj;
}

void mm_link_page_obj_ptr(unsigned int page_index, void * ptr)
{
    mm_global_area()->global_pages[page_index].obj = ptr;
}

mm_area_t * mm_area_from_addr(uint64_t addr)
{
    unsigned int area_index;
    mm_global_area_t * global_area = mm_global_area();

    addr = ALIGN_DOWN(addr, MM_AREA_SIZE);
    area_index = MM_AREA_INDEX(addr);
    if (area_index >= global_area->area_count) {
        DEBUG("Area index from addr exceedes global areas");
        return NULL;
    }

    return &global_area->global_areas[area_index];
}

mm_area_t * mm_find_free_area(unsigned int memorder)
{   
    ll_node_t * node;
    mm_global_area_t * global_area = mm_global_area();
    for (unsigned int free_memorder = memorder; free_memorder < MM_MAX_ORDER + 1; free_memorder++) {
        node = MM_GLOBAL_AREA_FREE_AREA(free_memorder);
        if (node)
            return (mm_area_t*)node->sll.data;
    }

    return NULL;
}

unsigned int mm_get_page_index(mm_page_t * page)
{
    return ((uint64_t)page) - (((uint64_t)mm_global_area()->global_pages) / sizeof(mm_page_t));
}

int mm_reserve_area(unsigned int area_index)
{
    mm_area_t * area = mm_area_from_addr(area_index * MM_AREA_SIZE);
}

int mm_area_init(mm_global_area_t * global_area, mm_area_t * area, unsigned int start_page_index)
{   
    kalloc_buddy_t * sibling;
    unsigned int buddy_num;
    
    ASSERT_PANIC(global_area && area, "MM area init nulls found");
    memset(area, 0, sizeof(mm_area_t));

    area->free_page_num = MM_AREA_SIZE / PAGE_SIZE;
    area->start_page_index = start_page_index;
    area->phys_addr_start = start_page_index * PAGE_SIZE;
    area->buddies = &global_area->global_buddies[start_page_index / 2];
    buddy_num = area->free_page_num / 2;

    DEBUG_DATA_DIGIT("Area init i=", MM_AREA_STRUCT_INDEX(area));
    DEBUG_DATA_DIGIT("buddy num=", buddy_num);

    for (unsigned int i = 0; i < MM_MAX_ORDER + 1; i++) {
        /* Init the list to keep track of free buddys in this area. */
        ll_head_init(&area->free_buddy_list[i], LIST_NODE_T);
        ll_node_init(&area->global_area_nodes[i], (void*)area, SLL_NODE_T);
    }

    // Init the first two buddies that make up the 8MB area size range
    kalloc_init_buddy(area, &area->buddies[0], MM_MAX_ORDER, 1);
    sibling = kalloc_get_buddy_sibling(&area->buddies[0]);
    kalloc_init_buddy(area, sibling, MM_MAX_ORDER, 1);

    return 0;
}

/* Reserve the early memory up to the early heap top, which should be the
 * top of the early memory space, with all code and data being placed below it. */
static void mm_reserve_early_mem(mmu_mem_map_t * phys_mem_map, uint64_t early_heap_top)
{  
    int ret;
    uint64_t device_mem_start;
    mm_area_t * last_area;
    unsigned int area_end;
    unsigned int reserve_page_num = ALIGN_UP(early_heap_top, PAGE_SIZE) / PAGE_SIZE;

    for (int i = 0; i < reserve_page_num; i++) {
        ret = kalloc_page_reserve_pages(i * PAGE_SIZE, 0, 0);
        if (ret) {
            DEBUG_PANIC("Reserve pages failed on initial mem reserve");
            return;
        }
    }

    device_mem_start = phys_mem_map[EARLY_MEM_MAP_DEVICE_MEM_START].start_addr;
    last_area = &mm_global_area()->global_areas[mm_global_area()->area_count - 1];

    /* If our last area happens to overlap with innacessible device memory,
     * we need to reserve those pages so we don't use them in our allocators. */
    if (!VAL_IN_RANGE(device_mem_start, last_area->phys_addr_start, MM_AREA_SIZE))
        return;

    area_end = (last_area->phys_addr_start + MM_AREA_SIZE) / PAGE_SIZE;
    for (unsigned int i = (device_mem_start / PAGE_SIZE); i < area_end; i++) {
        ret = kalloc_page_reserve_pages(i * PAGE_SIZE, 0, 0);
        if (ret) {
            DEBUG_PANIC("Reserve pages failed on initial mem reserve");
            return;
        }
    }
}

void mm_init()
{
    DEBUG("---MM INIT START--");

    size_t mem_size;
    unsigned int num_pages ;
    size_t global_pages_size_pages_num;
    unsigned int area_num;
    mmu_mem_map_t * phys_mem_map;
    int ret;

    ASSERT_PANIC(mm_early_is_intialized(), "Early mm is not initialized");

    phys_mem_map = mm_early_get_memmap();

    mem_size = phys_mem_map[0].size;
    /* We are aligning the memory space up to the AREA_SIZE which
     * might overlap with innaccesible memory, will need to take care to reserve
     * any pages that fall in this boundary */
    mem_size = ALIGN_UP(mem_size, MM_AREA_SIZE);

    num_pages = mem_size / PAGE_SIZE;
    global_pages_size_pages_num = (num_pages * sizeof(mm_page_t)) / PAGE_SIZE;
    area_num = mem_size / MM_AREA_SIZE;

    DEBUG_FUNC_DIGIT("-Global area page num=", num_pages);
    DEBUG_FUNC_DIGIT("-Global area pages struct size pages=", global_pages_size_pages_num);
    DEBUG_FUNC_DIGIT("-Global area sub area num=", area_num);
    DEBUG_FUNC_DIGIT("-Num buddies=", num_pages / 2);
    DEBUG_FUNC_DIGIT("-Buddies array size=", (num_pages / 2) * sizeof(kalloc_buddy_t));
    DEBUG_FUNC_DIGIT("-MM area size=", MM_AREA_SIZE);
    DEBUG_FUNC_DIGIT("-Memorder 10 size=", MM_MEMORDER_TO_PAGES(MM_MAX_ORDER) * PAGE_SIZE);

    /* Init the global area struct. */
    memset(&global_area, 0, sizeof(mm_global_area_t));
    spinlock_init(&global_area.lock);
    
    for (int i = 0; i < MM_MAX_ORDER + 1; i++) {
        ll_head_init(&global_area.free_areas_list[i], SLL_NODE_T);
    }

    /* Init all the global pages over the mem space. */
    global_area.global_pages = (mm_page_t *)page_data_alloc(global_pages_size_pages_num);
    global_area.page_count = num_pages;
    memset(global_area.global_pages, 0, global_pages_size_pages_num * sizeof(mm_page_t));

    /* Init the areas over the memory space. */
    align_mem(PAGE_SIZE);
    global_area.global_areas = (mm_area_t *)data_alloc(sizeof(mm_area_t) * area_num);
    memset(global_area.global_areas, 0, sizeof(mm_area_t) * area_num);

    align_mem(PAGE_SIZE);
    global_area.global_buddies = (kalloc_buddy_t *)data_alloc((num_pages / 2) * sizeof(kalloc_buddy_t));
    memset(global_area.global_buddies, 0, (num_pages / 2) * sizeof(kalloc_buddy_t));

    for (unsigned int i = 0; i < area_num; i++) {
        ret = mm_area_init(&global_area, &global_area.global_areas[i], (i * MM_AREA_SIZE)/PAGE_SIZE);
        if (ret) {
            DEBUG_PANIC("MM area init failed");
            return;
        }
    }

    global_area.area_count = area_num;

    mm_initialized = 1;

    mm_reserve_early_mem(phys_mem_map, (uint64_t)mm_early_get_heap_top());

    DEBUG("---MM INIT DONE---");
}
