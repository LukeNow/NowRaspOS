#ifndef __KALLOC_SLAB_H
#define __KALLOC_SLAB_H

#include <common/common.h>
#include <common/linkedlist.h>
#include <kernel/mmu.h>

/* kalloc_slab flags */
/* This slab was externally allocated outside of the page allocator or should not be freed until done explicitly. */
#define KALLOC_SLAB_EXTERN_ALLOC_F (1 << 1)

/* Pointer to the free index list, which should be directly after the struct in memory. */
#define KALLOC_SLAB_FREE_INDEXS(slab) ((unsigned int *)((kalloc_slab_t*)(slab) + 1))
/* Macro to extract the current cache list this slab belongs to, or any data we store in the ll node. */
#define KALLOC_SLAB_CURR_LIST_P(slab) ((ll_head_t *)((slab)->node.data))

#define KALLOC_SLAB_VERIFY(slab) ASSERT_PANIC(slab, "Slab ptr is null"); \
                        ASSERT_PANIC(slab->obj_size, "Slab obj size nonzer0"); \
                        ASSERT_PANIC(((uint64_t)slab->mem_ptr & PAGE_MASK) == ((uint64_t)slab  & PAGE_MASK), "Slab memptr is not in slab ptr range");

/* Very similar to the slab alloc linux uses. With the free indexs being the buf_ctl scheme linux also uses for updating
 * and getting free_indexs. */
typedef struct kalloc_slab {
    unsigned int max_num;
    unsigned int num;
    size_t obj_size;
    void * mem_ptr;
    sll_node_t node;
    flags_t flags;
    unsigned int mem_page_num;
    unsigned int free_index;
    unsigned int * free_indexs;
} kalloc_slab_t __attribute__ ((aligned (8)));

/* Size of the struct for the desired object number. */
size_t kalloc_slab_struct_size(unsigned int obj_num, size_t obj_size);
/* Size of the memory taken by the slab struct and the objects alloced after it. */
size_t kalloc_slab_total_size(unsigned int obj_num, size_t obj_size);
/* The amount of objects that fit into a given mem_size, not taking into account inline structs. */
unsigned int kalloc_slab_obj_num(size_t obj_size, size_t mem_size);
/* The number of objs in the slab that can fit alongside the inlined slab struct. */
unsigned int kalloc_slab_inline_obj_num_pages(size_t obj_size, unsigned int page_num);
/* Return either the passed in slab object to be initialized or the slab object initialized in the mem_ptr mem region. */
kalloc_slab_t * kalloc_slab_init(kalloc_slab_t * slab, void * mem_ptr, unsigned int mem_num_pages, size_t obj_size, flags_t flags);
void * kalloc_slab_alloc(kalloc_slab_t * slab);
int kalloc_slab_free(kalloc_slab_t * slab, void * obj);

#endif