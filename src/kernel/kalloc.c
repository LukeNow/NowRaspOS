#include <stddef.h>
#include <stdint.h>
#include <common/common.h>
#include <common/assert.h>
#include <common/linkedlist.h>
#include <kernel/kalloc.h>
#include <kernel/mm.h>
#include <common/lock.h>
#include <common/atomic.h>
#include <common/string.h>
#include <common/bits.h>
#include <common/math.h>
#include <kernel/kalloc_page.h>
#include <kernel/mmu.h>

#define KALLOC_ENTRY_NUM 8
#define KALLOC_MAX_ENTRY_ALLOC 2048
#define KALLOC_MAX_ALLOC_SIZE (MM_MEMORDER_TO_PAGES(MM_MAX_ORDER) * PAGE_SIZE)

DEFINE_SPINLOCK(lock);
static unsigned int kalloc_initialized = 0;
static kalloc_cache_t entry_cache[KALLOC_ENTRY_NUM];

typedef struct kalloc_entry {
    size_t size;
    kalloc_cache_t * cache;
    unsigned int slab_init_page_num;
    unsigned int alloc_num;
} kalloc_entry_t;

static kalloc_entry_t entries[] = {
    {16, &entry_cache[0], 2, 0},
    {32, &entry_cache[1], 2, 0},
    {64, &entry_cache[2], 2, 0},
    {128, &entry_cache[3], 4, 0}, 
    {256, &entry_cache[4], 4, 0}, 
    {512, &entry_cache[5], 4, 0}, 
    {1024, &entry_cache[6], 8, 0}, 
    {2048, &entry_cache[7], 8, 0}
};

static uint64_t normalize_addr(uint64_t addr)
{
    return addr & ~MMU_UPPER_ADDRESS;
}

static int get_entry_num_from_size(size_t size)
{
    for (int i = 0; i < KALLOC_ENTRY_NUM; i++) {
        if (size <= entries[i].size) {
            return i;
        }
    }

    return KALLOC_ENTRY_NUM - 1;
}

static int get_entry_num_from_cache(kalloc_cache_t * cache)
{
    for (int i = 0; i < KALLOC_ENTRY_NUM; i++) {
        if (cache == entries[i].cache) {
            return i;
        }
    }

    return -1;
}

static kalloc_cache_t * get_cache_from_addr(void * addr)
{
    kalloc_cache_t * cache;
    unsigned int page_index = PAGE_INDEX_FROM_PTR(addr);

    cache = mm_get_page_obj_ptr(page_index);
    /* If the page_obj_ptr is not a valid address, it might be invalid or it is a page entry address. */
    if ((uint64_t) cache < PAGE_SIZE) {
        return NULL;
    }

    return cache;
}

static unsigned int cache_expand(kalloc_cache_t * cache, unsigned int entry_num)
{
    kalloc_slab_t * slab;
    void * slab_mem;

    slab_mem = (void *)kalloc_page_alloc_pages(mm_pages_to_memorder(entries[entry_num].slab_init_page_num), 0);

    ASSERT_PANIC(slab_mem, "Slab mem alloc failed");

    slab = kalloc_cache_add_slab_pages(entries[entry_num].cache, slab_mem, 
                                    entries[entry_num].slab_init_page_num);
    ASSERT_PANIC(slab, "Slab add failed.");

    return 0;
}

static unsigned int check_cache_needs_expand(kalloc_cache_t * cache)
{
    // TODO implement some sort of threshold system so we can preexpand
    if (cache->num != cache->max_num)
        return 0;

    return 1; 
}

void * kalloc_pages(unsigned int page_num, flags_t flags)
{
    void * ptr = NULL;

    ASSERT_PANIC(page_num, "Kalloc_pages page num is 0");

    page_num = math_align_power2_64(page_num);

    lock_spinlock(&lock);

    ptr = (void *)kalloc_page_alloc_pages(mm_pages_to_memorder(page_num), flags);
    if (!ptr) {
        DEBUG_PANIC("Kalloc pages alloc pages failed.");
        goto kalloc_pages_exit;
    }

kalloc_pages_exit:
    lock_spinunlock(&lock);

    ptr = (void *)((uint64_t)ptr | MMU_UPPER_ADDRESS);

    return ptr;
}

int kalloc_free_pages(void * page_ptr, flags_t flags)
{
    int ret = 0;

    lock_spinlock(&lock);

    ret = kalloc_page_free_pages((uint64_t)page_ptr, flags);

    lock_spinunlock(&lock);
    
    ASSERT_PANIC(!ret, "Kalloc free pages failed.");
    return ret;
}

void * kalloc_alloc(size_t size, flags_t flags)
{   
    kalloc_cache_t * cache;
    unsigned int page_num;
    int entry_num;
    void * obj;

    int ret = 0;
    
    ASSERT_PANIC(kalloc_initialized, "Kalloc is not initialized");

    if (size > KALLOC_MAX_ENTRY_ALLOC) {
        page_num = ALIGN_UP(size, PAGE_SIZE) / PAGE_SIZE;
        DEBUG_DATA_DIGIT("Kalloc allocing from pages page_num=", page_num);
        return kalloc_pages(page_num, flags);
    }

    lock_spinlock(&lock);

    DEBUG_DATA_DIGIT("KALLOC alloc with size=", size);
    entry_num = get_entry_num_from_size(size);

    DEBUG_DATA_DIGIT("Kalloc entry_num=", entry_num);
    cache = entries[entry_num].cache;

    if (check_cache_needs_expand(cache)) {
        ret = cache_expand(cache, entry_num);
        if (ret) {
            DEBUG_THROW("Kalloc entry cache expand failed.");
            obj = NULL;
            goto kalloc_alloc_exit;
        }
    }

    obj = kalloc_cache_alloc(cache);
    if (!obj) {
        DEBUG_THROW("Kalloc entry alloc failed.");
        goto kalloc_alloc_exit;
    }

    entries[entry_num].alloc_num++;

kalloc_alloc_exit:
    lock_spinunlock(&lock);

    obj = (void *)((uint64_t)obj | MMU_UPPER_ADDRESS);

    return obj;
}

int kalloc_free(void * obj, flags_t flags)
{
    kalloc_cache_t * cache;
    unsigned int page_num;
    int entry_num;
    int ret = 0;

    ASSERT_PANIC(kalloc_initialized, "Kalloc is not initialized");

    obj = (void *)((uint64_t)obj & ~MMU_UPPER_ADDRESS);

    cache = get_cache_from_addr(obj);
    if (!cache) {
        DEBUG("Linked cache not found, attempting to free page. ");
        if (!IS_ALIGNED((uint64_t)obj, PAGE_SIZE)) {
            DEBUG_PANIC("Suspected page pointer is not aligned.");
        }

        return kalloc_page_free_pages((uint64_t)obj, flags);
    }

    lock_spinlock(&lock);

    entry_num = get_entry_num_from_cache(cache);
    if (entry_num == -1) {
        DEBUG_PANIC("Could not find entry from linked cache.");
        ret = 1;
        goto kalloc_free_exit;
    }
    
    ret = kalloc_cache_free(cache, obj);
    if (ret) {
        DEBUG_PANIC("Kalloc could not free obj from cache.");
        goto kalloc_free_exit;
    }

    entries[entry_num].alloc_num--;

kalloc_free_exit:
    lock_spinunlock(&lock);

    return ret;
}

int kalloc_init()
{   
    DEBUG("-- Kalloc init --");

    kalloc_cache_t * cache;
    int ret = 0;

    ASSERT_PANIC(mm_is_initialized(), "MM is not initialized");

    for (int i = 0; i < KALLOC_ENTRY_NUM; i++) {
        ret = kalloc_cache_init(entries[i].cache, entries[i].size, 
                                entries[i].slab_init_page_num, NULL, NULL, 
                                KALLOC_CACHE_NO_EXPAND_F | KALLOC_CACHE_NO_SHRINK_F);
        ASSERT_PANIC(!ret, "Cache failed to initialize.");

        ret = cache_expand(cache, i);
        ASSERT_PANIC(!ret, "Initial cache expand failed.");
    }

    kalloc_initialized = 1;

    DEBUG("-- Kalloc init done --");
    return 0;
}