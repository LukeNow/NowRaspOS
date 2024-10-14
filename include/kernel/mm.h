#ifndef __MM_H
#define __MM_H

#include <stdint.h>
#include <stddef.h>
#include <common/bitmap.h>
#include <common/bits.h>
#include <common/linkedlist.h>
#include <common/lock.h>

/* The max order of page orders that we keep track of. Similiar to the linux max order size. 
 * Areas can be represented with 2 64bit bitmaps */
#define MM_MAX_ORDER 10
#define MM_INVALID_ORDER ((unsigned int)~0)
#define MM_MAX_INDEX_ORDER (MM_MAX_ORDER + 1)
#define MM_AREA_SIZE (PAGE_SIZE << (MM_MAX_ORDER + 1)) //8MB

#define MM_AREA_INDEX(ADDR) ((ADDR)/ MM_AREA_SIZE)
#define MM_AREA_FROM_ADDR(ADDR) (&mm_global_area()->global_areas[MM_AREA_INDEX(ADDR)])
#define MM_AREA_STRUCT_INDEX(AREA) ((AREA)->phys_addr_start / MM_AREA_SIZE)

#define MM_MEMORDER_SIZE(MEMORDER) (PAGE_SIZE << (MEMORDER))
#define MM_MEMORDER_MASK(MEMORDER) (~(MM_MEMORDER_SIZE(MEMORDER) - 1))

#define MM_MEMORDER_TO_PAGES(MEMORDER) (1 << (MEMORDER))

#define MM_AREA_FREE_BUDDY(AREA, MEMORDER) ((list_node_t *)ll_peek_first(&(AREA)->free_buddy_list[MEMORDER]))
#define MM_GLOBAL_AREA_FREE_AREA(MEMORDER) ((sll_node_t *)ll_peek_first(&mm_global_area()->free_areas_list[MEMORDER]))

#define MM_PAGE_VALID (1 << 0)

typedef struct mm_addr_map {
    uint64_t virt_addr;
} mm_addr_map_t;

typedef struct kalloc_buddy {
    unsigned int buddy_memorder;
    list_node_t buddy_node;
} kalloc_buddy_t;

typedef struct mm_page {
    /* Virtual mapping info. */
    mm_addr_map_t mapping;
    /* Kalloc obj i.e. slab ptr or other memory objs for easier lookups. */
    void * obj;
    flags_t page_flags;
} mm_page_t __attribute__ ((aligned (8)));

typedef struct mm_area {
    unsigned int free_page_num;
    unsigned int start_page_index;
    uint64_t phys_addr_start;
    kalloc_buddy_t * buddies;
    /* Simple ll list for keeping track of free buddies. */
    ll_head_t free_buddy_list[MM_MAX_INDEX_ORDER];
    /* Nodes for linking to global area count lists. Area ptr is embedded in data. */
    sll_node_t global_area_nodes[MM_MAX_INDEX_ORDER];
} mm_area_t;

typedef struct mm_global_area {
    mm_page_t * global_pages;
    unsigned int page_count;
    mm_area_t * global_areas;
    kalloc_buddy_t * global_buddies;
    unsigned int area_count;
    ll_head_t free_areas_list[MM_MAX_INDEX_ORDER];
    spinlock_t lock;
} mm_global_area_t;


int mm_page_is_valid(unsigned int page_index);
int mm_pages_are_valid(unsigned int start_page_index, unsigned int page_num);
void mm_mark_page_valid(unsigned int page_index);
void mm_mark_pages_valid(unsigned int start_page_index, unsigned int page_num);
int mm_pages_are_free(unsigned int start_page_index, unsigned int page_num);
void mm_mark_pages_free(unsigned int start_page_index, unsigned int page_num);
void * mm_get_page_obj_ptr(unsigned int page_index);
void mm_link_page_obj_ptr(unsigned int page_index, void * ptr);

unsigned int mm_pages_to_memorder(unsigned int page_num);
mm_area_t * mm_find_free_area(unsigned int memorder);
mm_area_t * mm_area_from_addr(uint64_t addr);
mm_global_area_t * mm_global_area();

int mm_is_initialized();
int mm_area_init(mm_global_area_t * global_area, mm_area_t * area, unsigned int start_page_index);
void mm_init();

#endif