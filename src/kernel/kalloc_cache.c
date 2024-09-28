#include <stddef.h>
#include <stdint.h>

#include <common/common.h>
#include <common/string.h>
#include <common/assert.h>
#include <common/lock.h>
#include <kernel/mm.h>
#include <kernel/kalloc_slab.h>
#include <kernel/kalloc_cache.h>

static int add_remove_cache_list(ll_node_t  * to_cache_list, ll_node_t * from_cache_list, kalloc_slab_t * slab)
{
    KALLOC_SLAB_VERIFY(slab);

    if (from_cache_list)  {
        if (ll_delete_node(from_cache_list, &slab->node)) {
            DEBUG_THROW("Error in removing slab from free list");
            return 1;
        }
    }

    if (to_cache_list) {
        // Initialize which list we are in in the ll_nodes.data
        ll_node_init(&slab->node, to_cache_list);
        if (ll_push_list(to_cache_list, &slab->node))  {
            DEBUG_THROW("Error in adding slab to full list");
            return 1;
        }
    }

    return 0;
}

static ll_node_t * get_curr_slab_list(kalloc_cache_t * cache, kalloc_slab_t * slab)
{
    KALLOC_SLAB_VERIFY(slab);

    ll_node_t * curr_list = KALLOC_SLAB_CURR_LIST_P(slab);

    if (!curr_list || (curr_list != &cache->free_list && curr_list != &cache->partial_list && curr_list != &cache->full_list)) {
        DEBUG_THROW("Slab curr list does not map onto any cache list head");
        return NULL;
    }

    return curr_list;
}

static kalloc_slab_t * get_free_slab(kalloc_cache_t * cache)
{
    kalloc_slab_t * slab;
    ll_node_t * slab_node;

    if (!(slab_node = ll_peek_first_list(&cache->partial_list)) && !(slab_node = ll_peek_first_list(&cache->free_list)))  {
        DEBUG("No free slabs in both partial and free list found");
        return NULL;
    }

    slab = STRUCT_P(slab_node, kalloc_slab_t, node);
    ASSERT_PANIC(slab->max_num, "Max num is 0, slab not free or malformed");
    ASSERT_PANIC(slab->num != slab->max_num, "Slab is full but in free list");

    KALLOC_SLAB_VERIFY(slab);
    return slab;
}

static kalloc_slab_t * _get_slab_from_addr(ll_node_t * list, void * obj)
{
    ll_node_t * node_p;
    kalloc_slab_t * slab;

    LL_ITERATE_LIST(list, node_p) {
        slab = STRUCT_P(node_p, kalloc_slab_t, node);
        KALLOC_SLAB_VERIFY(slab);
        if (PTR_IN_RANGE(obj, slab->mem_ptr, slab->obj_size * slab->max_num)) {
            return slab;
        }
    }

    return NULL;
}

static kalloc_slab_t * get_slab_from_addr(kalloc_cache_t * cache, void * obj)
{
    kalloc_slab_t * slab;

    slab = _get_slab_from_addr(&cache->full_list, obj);
    if (slab) {
        return slab;
    }

    slab = _get_slab_from_addr(&cache->partial_list, obj);
    if  (slab) {
        return slab;
    }

    // For debugging lets also check if its in the free list, we can disable this part later
    slab = _get_slab_from_addr(&cache->free_list, obj);
    ASSERT_PANIC(slab, "Cache obj found in free list");

    return NULL;
}

static int check_slab_and_update_list(kalloc_cache_t * cache, kalloc_slab_t * slab)
{
    ll_node_t * to_list, *curr_list;
    
    KALLOC_SLAB_VERIFY(slab);
    ASSERT(cache && slab);

    curr_list = get_curr_slab_list(cache, slab);
    if (!curr_list) {
        DEBUG("Slab list malformed or not in cache");
        return 1;
    }

    if (slab->max_num == slab->num && curr_list != &cache->full_list) {
        to_list = &cache->full_list;
    } else if (!slab->num && curr_list != &cache->free_list) {
        to_list = &cache->free_list;
    } else if (curr_list != &cache->partial_list) {
        to_list = &cache->partial_list;
    } else  {
        return 0;
    }

    if (add_remove_cache_list(to_list, curr_list, slab)) {
        DEBUG_THROW("Swap list failed");
        return 1;
    }

    return 0;
}

int kalloc_cache_add_slab(kalloc_cache_t * cache, kalloc_slab_t * slab)
{
    ASSERT_PANIC(slab->num == 0, "Slab is being added that is not free");

    unsigned int page_index;

    int ret = 0;

    if (add_remove_cache_list(&cache->free_list, NULL, slab)) {
        DEBUG_THROW("Adding cache list failed");
        return 1;
    }
    
    cache->max_num += slab->max_num;
    cache->page_num += slab->mem_page_num;

    if (cache->flags & KALLOC_CACHE_NO_LINK_F)
        return ret;

    page_index = PAGE_INDEX_FROM_PTR(slab->mem_ptr);
    for (unsigned int i = 0; i < slab->mem_page_num; i++) {
        mm_link_page_obj_ptr(page_index + i, (void*)cache);
    }

    return ret;
}

kalloc_slab_t * kalloc_cache_add_slab_pages(kalloc_cache_t * cache, void * page_ptr, unsigned int page_num)
{   
    kalloc_slab_t * slab;

    slab = kalloc_slab_init(NULL, page_ptr, page_num, cache->obj_size, 0);
    if (!slab) {
        DEBUG_THROW("Slab returned is NULL");
        return slab;
    }
    
    if (kalloc_cache_add_slab(cache, slab)) {
        return NULL;
    }

    KALLOC_SLAB_VERIFY(slab);

    return slab;
}

int kalloc_cache_remove_slab(kalloc_cache_t * cache, kalloc_slab_t * slab)
{
    unsigned int page_index;
    ll_node_t * curr_list;
    int ret = 0;

    KALLOC_SLAB_VERIFY(slab);

    curr_list = get_curr_slab_list(cache, slab);
    if (!curr_list) {
        DEBUG("Slab list malformed or not in cache");
        return 1;
    }

    if (curr_list != &cache->free_list || slab->num)  {
        DEBUG("Slab is being remove when not in free list or has active objects");
        return 1;
    }

    ASSERT_PANIC(cache->max_num >= slab->max_num, "Cache max num less than the total found in slab, out of sync?");

    cache->max_num -= slab->max_num;
    cache->page_num -= slab->mem_page_num;

    if (cache->flags & KALLOC_CACHE_NO_LINK_F)
        return ret;

    page_index = (uint64_t)slab->mem_ptr / PAGE_SIZE;
    for (unsigned int i = 0; i < slab->mem_page_num; i++) {
        mm_link_page_obj_ptr(page_index + i, NULL);
    }

    // TODO do we free the slab here as well?

    return ret;
}

void * kalloc_cache_alloc(kalloc_cache_t * cache)
{
    kalloc_slab_t * slab;
    void * obj;
    
    ASSERT_PANIC(cache, "Cache is NULL");
    
    if (cache->num == cache->max_num) {
        /*
        if (cache->flags & KALLOC_CACHE_NO_EXPAND_F || !cache->page_allocator) {
            DEBUG_THROW("Cache is full and no expand set");
            goto cache_alloc_fail;
        }

        ret = cache->page_allocator(cache->slab_init_page_num, cache->flags);

        if (ret || cache->num == cache->max_num) {
            DEBUG_THROW("Allocator could not expand cache.");
            goto cache_alloc_fail;
        }
        */
       DEBUG_THROW("Kalloc cache is full, can't alloc.");
       return NULL;
    }

    slab = get_free_slab(cache);
    if (!slab) {
        DEBUG_THROW("Slab is NULL");
        goto cache_alloc_fail;
    }

    KALLOC_SLAB_VERIFY(slab);
    obj = kalloc_slab_alloc(slab);
    if (!obj) {
        DEBUG_THROW("Slab obj is NULL");
        goto cache_alloc_fail;
    }

    if (check_slab_and_update_list(cache, slab)) {
        DEBUG_THROW("Checking and swapping slab failed. No movement or invalid list?");
        goto cache_alloc_fail;
    }

    cache->num ++;
    return obj;

cache_alloc_fail:
    return NULL;
}

int kalloc_cache_free(kalloc_cache_t * cache, void * obj)
{
    kalloc_slab_t * slab;
    int ret = 0;
    ASSERT(cache && obj);

    if (cache->num == 0) {
        DEBUG_THROW("Kalloc cache is empty, can't free.");
        return 1;
    }

    slab = get_slab_from_addr(cache, obj);
    if (slab) {
        goto cache_free_obj;
    } 

    DEBUG_THROW("Obj could not be found from any list");
    ret = 1;
    goto cache_free_exit;

cache_free_obj:
    KALLOC_SLAB_VERIFY(slab);

    ret = kalloc_slab_free(slab, obj);

    if (!ret && check_slab_and_update_list(cache, slab)) {
        DEBUG_THROW("Checking and swapping slab failed. No movement or invalid list?");
        ret = 1;
    }

    cache->num --;
cache_free_exit:
    return ret;
}

int kalloc_cache_init(kalloc_cache_t * cache, size_t obj_size, 
                        unsigned int slab_init_page_num,
                        void *(*page_allocator)(unsigned int, flags_t), 
                        int (*page_destructor)(void *, unsigned int, flags_t), 
                        flags_t flags)
{
    ASSERT(cache && obj_size);
    
    memset(cache, 0, sizeof(kalloc_cache_t));

    cache->obj_size = obj_size;
    cache->slab_init_page_num = slab_init_page_num;
    cache->flags = flags;

    lock_init(&cache->lock);
    ll_root_init(&cache->free_list);
    ll_root_init(&cache->partial_list);
    ll_root_init(&cache->full_list);

    if (page_allocator) {
        ASSERT(!(cache->flags & KALLOC_CACHE_NO_EXPAND_F));
        cache->page_allocator = page_allocator;
    }

    if (page_destructor) {
        ASSERT(!(cache->flags & KALLOC_CACHE_NO_SHRINK_F));
        cache->page_destructor = page_destructor;
    }

    return 0;
}