#ifndef __KALLOC_CACHE_H
#define __KALLOC_CACHE_H

#include <common/common.h>
#include <common/linkedlist.h>
#include <kernel/kalloc_slab.h>
#include <common/lock.h>

/* kalloc_cache flags */
/* Do not expand/shrink when full. */
#define KALLOC_CACHE_NO_EXPAND_F (1 << 1)
#define KALLOC_CACHE_NO_SHRINK_F (1 << 2)
#define KALLOC_CACHE_NO_LINK_F (1 << 3)

typedef struct kalloc_cache {
    unsigned int max_num;
    unsigned int num;
    size_t obj_size;
    ll_node_t free_list;
    ll_node_t partial_list;
    ll_node_t full_list;
    unsigned int slab_init_page_num;
    unsigned int page_num;
    void *(*page_allocator)(unsigned int, flags_t);
    int (*page_destructor)(void *, unsigned int, flags_t);
    spinlock_t lock;
    flags_t flags;
} kalloc_cache_t;

int kalloc_cache_init(kalloc_cache_t * cache, size_t obj_size, 
                        unsigned int slab_init_page_num,
                        void *(*page_allocator)(unsigned int, flags_t), 
                        int (*page_destructor)(void *, unsigned int, flags_t), 
                        flags_t flags);
int kalloc_cache_add_slab(kalloc_cache_t * cache, kalloc_slab_t * slab);
kalloc_slab_t * kalloc_cache_add_slab_pages(kalloc_cache_t * cache, void * page_ptr, unsigned int page_num);
int kalloc_cache_remove_slab(kalloc_cache_t * cache, kalloc_slab_t * slab);
void * kalloc_cache_alloc(kalloc_cache_t * cache);
int kalloc_cache_free(kalloc_cache_t * cache, void * obj);

#endif