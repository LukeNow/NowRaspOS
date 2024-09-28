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

DEFINE_SPINLOCK(mm_lock);

static mm_global_area_t global_area;

static int mm_initialized = 0;

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
            return (mm_area_t*)node->data;
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
        ll_root_init(&area->free_buddy_list[i]);
        ll_node_init(&area->global_area_nodes[i], (void*)area);
    }

    /*
    for (unsigned int i = 0; i < buddy_num; i++) {
        kalloc_init_buddy(area, &area->buddies[i], MM_INVALID_ORDER, 0);
    }*/

    // Init the first two buddies that make up the 8MB area size range
    kalloc_init_buddy(area, &area->buddies[0], MM_MAX_ORDER, 1);
    sibling = kalloc_get_buddy_sibling(&area->buddies[0]);
    kalloc_init_buddy(area, sibling, MM_MAX_ORDER, 1);

    return 0;
}

int mm_init(size_t mem_size, uint64_t *mem_start_addr)
{ 
    unsigned int num_pages = mem_size / PAGE_SIZE;
    size_t global_pages_size_pages_num = (num_pages * sizeof(mm_page_t)) / PAGE_SIZE;
    unsigned int area_num = mem_size / MM_AREA_SIZE;
    int ret = 0;

    DEBUG("---MM INIT START--");


    ASSERT_PANIC(mm_early_is_intialized(), "Early mm is not initialized");

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
        ll_root_init(&global_area.free_areas_list[i]);
    }

    /* Init all the global pages over the mem space. */
    global_area.global_pages = (mm_page_t *)early_page_data_alloc(global_pages_size_pages_num);
    global_area.page_count = num_pages;
    memset(global_area.global_pages, 0, global_pages_size_pages_num * sizeof(mm_page_t));

    /* Init the areas over the memory space. */
    align_early_mem(PAGE_SIZE);
    global_area.global_areas = (mm_area_t *)early_data_alloc(sizeof(mm_area_t) * area_num);
    memset(global_area.global_areas, 0, sizeof(mm_area_t) * area_num);

    align_early_mem(PAGE_SIZE);
    global_area.global_buddies = (kalloc_buddy_t *)early_data_alloc((num_pages / 2) * sizeof(kalloc_buddy_t));
    memset(global_area.global_buddies, 0, (num_pages / 2) * sizeof(kalloc_buddy_t));

    for (unsigned int i = 0; i < area_num; i++) {
        ret = mm_area_init(&global_area, &global_area.global_areas[i], (i * MM_AREA_SIZE)/PAGE_SIZE);
        if (ret) {
            ASSERT(0);
            DEBUG("MM area init failed");
            return 1;
        }
    }

    global_area.area_count = area_num;

    mm_initialized = 1;

    DEBUG("---MM INIT DONE--");
    return 0;
}
