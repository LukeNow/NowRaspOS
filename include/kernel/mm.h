#ifndef __MM_H
#define __MM_H

#include <stdint.h>
#include <stddef.h>
#include <common/bitmap.h>
#include <common/bits.h>
#include <common/linkedlist.h>
#include <kernel/lock.h>

/* The max order of page orders that we keep track of. Similiar to the linux max order size. 
 * Areas can be represented with 2 64bit bitmaps */
#define MM_MAX_ORDER 10
#define MM_MAX_INDEX_ORDER (MM_MAX_ORDER + 1)

#define MM_BITMAPS_PER_AREA ((MM_BITMAP_MAX_INDEX + 1)/ BITMAP_BITS_PER_BITMAP_ENTRY)

#define MM_MEMORDER_SIZE(MEMORDER) (PAGE_SIZE << MEMORDER)
#define MM_MEMORDER_MASK(MEMORDER) (~(MM_MEMORDER_SIZE(MEMORDER) - 1))
#define MM_MEMORDER_INDEX(ADDR, MEMORDER) ((ADDR & MM_MEMORDER_MASK(MEMORDER)) >> (MEMORDER + PAGE_BIT_OFF))
#define MM_MEMORDER_LOCAL_INDEX(ADDR, MEMORDER) (MM_MEMORDER_INDEX(ADDR, MEMORDER + 1) % (1 << (MM_MAX_ORDER - MEMORDER)))

#define MM_GLOBAL_AREA_INDEX(ADDR) (MM_MEMORDER_INDEX(ADDR, MM_MAX_ORDER + 1))
#define MM_GLOBAL_PAGE_INDEX(ADDR) (MM_MEMORDER_INDEX(ADDR, 0))

#define MM_BITMAP_ENTRY_NUM(MEMORDER) (BITS(MM_MAX_ORDER) >> MEMORDER)
#define MM_BITMAP_LOCAL_INDEX(ADDR, MEMORDER) (MM_MEMORDER_LOCAL_INDEX(ADDR, MEMORDER) + MM_BITMAP_ENTRY_NUM(MEMORDER))
#define MM_BITMAP_GLOBAL_INDEX(ADDR, MEMORDER) (MM_BITMAP_LOCAL_INDEX(ADDR, MEMORDER) + MM_GLOBAL_AREA_INDEX(ADDR) * (2 << MM_MAX_ORDER))
#define MM_BITMAP_ENTRY_START(ADDR) (MM_GLOBAL_AREA_INDEX(ADDR) * MM_BITMAPS_PER_AREA)
#define MM_BITMAP_MEMORDER_OFFSET(INDEX, MEMORDER) (INDEX - MM_BITMAP_ENTRY_NUM(MEMORDER))
#define MM_BITMAP_MAX_INDEX (MM_BITMAP_ENTRY_NUM(0) + MM_BITMAP_ENTRY_NUM(0) + 1)

#define MM_MEMORDER_TO_PAGES(MEMORDER) (1 << MEMORDER)
#define MM_BUDDY_PAGE_OFFSET(INDEX, MEMORDER) (MM_BITMAP_MEMORDER_OFFSET(INDEX, MEMORDER) * MM_MEMORDER_TO_PAGES(MEMORDER) * 2)
#define MM_BUDDY_CHILD_PAGE_OFFSET(INDEX, MEMORDER, IS_RIGHT_CHILD) (MM_BUDDY_PAGE_OFFSET(INDEX, MEMORDER) + IS_RIGHT_CHILD * MM_MEMORDER_TO_PAGES(MEMORDER))

/* Because the lowest order in the bitmap represents a "buddy page" or a pair of pages,
 * the area and what it tracks would be 1 order above max order. */
#define MM_AREA_SIZE (MM_MEMORDER_SIZE(MM_MAX_ORDER + 1))
#define MM_AREA_INDEX(ADDR) (ADDR/ MM_AREA_SIZE)
#define MM_AREA_FROM_ADDR(ADDR) (&mm_global_area()->global_areas[MM_AREA_INDEX(ADDR)])
#define MM_AREA_STRUCT_INDEX(AREA) (AREA->phys_addr_start / MM_AREA_SIZE)

#define MM_AREA_FREE_BUDDY(AREA, MEMORDER) (ll_peek_first_list(&AREA->free_buddy_list[MEMORDER]))
#define MM_GLOBAL_AREA_FREE_AREA(MEMORDER) (ll_peek_first_list(&mm_global_area()->free_areas_list[MEMORDER]))

#define MM_BUDDY_NULL_INDEX ~0
#define MM_BUDDY_INDEX(BUDDY_NODE) ((unsigned int)((ll_node_t *)BUDDY_NODE->data))
#define MM_SET_BUDDY_INDEX(BUDDY_NODE, INDEX) ((ll_node_t *)BUDDY_NODE->data = (unsigned int)INDEX)
#define MM_BUDDY_LEFT_CHILD(INDEX) (2*(INDEX)+1)
#define MM_BUDDY_RIGHT_CHILD(INDEX) (2*(INDEX)+2)
#define MM_BUDDY_IS_RIGHT_CHILD(INDEX) (INDEX % 2 == 0)
#define MM_BUDDY_IS_LEFT_CHILD(INDEX) (INDEX % 2 == 1)
#define MM_BUDDY_PARENT(INDEX) (((INDEX)-1)/2)
#define MM_BUDDY_SIBLING(INDEX) (((INDEX-1)^1)+1)

#define MM_BUDDY_ADDR(AREA, INDEX, MEMORDER) ((AREA)->phys_addr_start + (MM_BUDDY_PAGE_OFFSET(INDEX, MEMORDER) * PAGE_SIZE))
#define MM_BUDDY_CHILD_ADDR(AREA, INDEX, MEMORDER) (MM_BUDDY_ADDR(AREA, INDEX, MEMORDER) + (MM_BUDDY_IS_RIGHT_CHILD(INDEX) ? (MM_MEMORDER_TO_PAGES(MEMORDER) * PAGE_SIZE) : 0))

typedef enum mm_buddy_child {
    mm_buddy_none = 0, 
    mm_buddy_left_child = 1,
    mm_buddy_right_child = 2,
    mm_buddy_either = 3
} mm_buddy_dir_t;

typedef struct mm_addr_map {
    uint64_t virt_addr;
} mm_addr_map_t;

#define MM_PAGE_VALID (1 << 0)

typedef struct mm_page {
    /* Virtual mapping info. */
    mm_addr_map_t mapping;
    /* Kalloc obj i.e. slab ptr or alloced page_num that we store for easier frees */
    void * kalloc_obj;
    flags_t page_flags;
} mm_page_t __attribute__ ((aligned (8)));

#define MM_FREE_AREA_NODE_INDEX(node) ((unsigned int)node->data)
#define MM_FREE_AREA_NODE_INDEX_SET(node, index) ((unsigned int)node->data = index)
typedef struct mm_area {
    unsigned int free_page_num; 
    unsigned int start_page_index;
    uint64_t phys_addr_start;
    bitmap_t * bitmap;
    /* The free bit_index into the bitmap is embedded in the ll_node's data. */
    ll_node_t free_buddy_list[MM_MAX_INDEX_ORDER];
    /* Nodes for linking to global area count lists. */
    ll_node_t global_area_nodes[MM_MAX_INDEX_ORDER];
} mm_area_t;

typedef struct mm_global_area {
    mm_page_t * global_pages;
    unsigned int page_count;
    bitmap_t * global_bitmap;
    mm_area_t * global_areas;
    unsigned int area_count;
    ll_node_t free_areas_list[MM_MAX_INDEX_ORDER];
    spinlock_t lock;
} mm_global_area_t;

int mm_early_is_intialized();
int mm_early_init();
void * mm_earlypage_alloc(int num_pages);
int mm_earlypage_shrink(int num_pages);

mm_global_area_t * mm_global_area();
int mm_page_is_valid(unsigned int page_index);
void mm_mark_page_valid(unsigned int page_index);
int mm_pages_are_free(uint64_t page_addr, unsigned int memorder);
void * mm_get_page_obj_ptr(unsigned int page_index);
void mm_link_page_obj_ptr(unsigned int page_index, void * ptr);
unsigned int mm_pages_to_memorder(unsigned int page_num);
mm_global_area_t * mm_global_area();
ll_node_t * mm_alloc_node(void * data);
void mm_free_node(ll_node_t * node);

int mm_is_initialized();
int mm_init(size_t mem_size, uint64_t *mem_start_addr);
uint64_t mm_alloc_pages(unsigned int memorder);
int mm_reserve_pages(uint64_t addr, unsigned int memorder);
int mm_free_pages(uint64_t addr, unsigned int memorder);

int mm_area_init(mm_global_area_t * global_area, mm_area_t * area, unsigned int start_page_index);

#endif