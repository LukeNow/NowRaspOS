#include <stddef.h>
#include <stdint.h>
#include <common/common.h>
#include <common/assert.h>
#include <common/string.h>
#include <kernel/kalloc_slab.h>
#include <kernel/mmu.h>

size_t kalloc_slab_free_list_size(unsigned int obj_num)
{
    return (sizeof(unsigned int) * (obj_num - 1));
}

// return the size of the slab struct for allocating 
size_t kalloc_slab_struct_size(unsigned int obj_num, size_t obj_size)
{
    // The struct with the free_indexs array included after the struct
    // The free indexs array is size (objnum - 1)
    // Align everything up to the obj size so that objs start on their own size byte boundary
    return (size_t)ALIGN_UP(sizeof(kalloc_slab_t) + kalloc_slab_free_list_size(obj_num), obj_size);
}

// return the size of the total mem size when in-lining the struct in the memory region
size_t kalloc_slab_total_size(unsigned int obj_num, size_t obj_size)
{
    return (size_t)(ALIGN_UP(kalloc_slab_struct_size(obj_num, obj_size), obj_size) + (obj_num * obj_size));
}

unsigned int kalloc_slab_obj_num(size_t obj_size, size_t mem_size)
{
    return ALIGN_DOWN(mem_size, obj_size) / obj_size;
}

// TODO make this more logarithmic or make better guesses
// return the number of objs that can fit within a page alongside the embedded slab struct
unsigned int kalloc_slab_inline_obj_num_pages(size_t obj_size, unsigned int num_pages)
{
    size_t total_size;
    size_t mem_size = num_pages * PAGE_SIZE;

    // Start the best guess at the number of objects with the struct and free list with 1 object
    // TODO we could make this much more efficient
    unsigned int num = kalloc_slab_obj_num(obj_size, mem_size);
    do {
        total_size = kalloc_slab_total_size(num, obj_size);
        num --;
    } while (num > 0 && total_size > mem_size);

    return num;
}

kalloc_slab_t * kalloc_slab_init(kalloc_slab_t * slab, void * mem_ptr, unsigned int mem_num_pages, size_t obj_size, flags_t flags)
{
    size_t struct_size;
    unsigned int num;
    
    ASSERT(obj_size % sizeof(uint32_t) == 0 && mem_num_pages);

    // We are allocating the struct embedded in the mem_region provided
    if (!slab) {
        memset(mem_ptr, 0, mem_num_pages * PAGE_SIZE);

        num = kalloc_slab_inline_obj_num_pages(obj_size, mem_num_pages);

        ASSERT_PANIC(num, "Calculated num of objects is 0");

        struct_size = kalloc_slab_struct_size(num, obj_size);

        slab = (kalloc_slab_t *)mem_ptr;
        slab->mem_ptr = (char*)mem_ptr + struct_size;

        ASSERT_PANIC((((uint64_t)slab->mem_ptr) + (slab->obj_size * slab->max_num)) < ((uint64_t)slab + PAGE_SIZE * mem_num_pages), 
                    "Slab mem out of range");
    // We are allocating from a struct provided and filling the mem_ptr region up to the max_num objects providied
    } else {
        num = kalloc_slab_obj_num(obj_size, PAGE_SIZE * mem_num_pages);
        // zero the struct provided
        memset(slab, 0, kalloc_slab_struct_size(num, obj_size));
        // Zero the object area
        memset(mem_ptr, 0, num * obj_size);

        slab->mem_ptr = mem_ptr;
    }

    slab->free_indexs = KALLOC_SLAB_FREE_INDEXS(slab);
    slab->free_index = 0;
    for (unsigned int i = 0; i < num - 1; i++) {
        // For every index the next index is free
        slab->free_indexs[i] = i + 1;
    }

    /* We are reserving one memory space for the overflow that occurs when we fully allocate 
     * the slab, so that slab->free_index is always pointing to an actual free index. */
    slab->max_num = num - 1;
    slab->obj_size = obj_size;
    slab->mem_page_num = mem_num_pages;
    
    slab->flags = flags;

    // either return the given slab or return the slab inlined in the mem_ptr region
    return slab;
}

void * kalloc_slab_alloc(kalloc_slab_t * slab)
{   
    ASSERT_PANIC(slab->num != slab->max_num, "Slab is already full");
    ASSERT_PANIC(slab->free_index <= slab->max_num, "Free index is not in max num range");
    ASSERT_PANIC(slab->free_indexs[slab->free_index] <= slab->max_num, "Free indexs are not in max num range");
    
    void * obj = slab->mem_ptr + slab->obj_size * slab->free_index;
    slab->free_index = slab->free_indexs[slab->free_index];
    slab->num++;

    ASSERT_PANIC(PTR_IN_RANGE(obj, slab->mem_ptr, slab->obj_size * (slab->max_num  + 1)), 
                "Alloced obj outside of mem range");
    return obj;
}

int kalloc_slab_free(kalloc_slab_t * slab, void * obj)
{   
    ASSERT_PANIC(slab->num, "Slab is all freed");

    ASSERT_PANIC(PTR_IN_RANGE(obj, slab->mem_ptr, slab->obj_size * (slab->max_num  + 1)), 
                "Obj outside of slab range.");

    unsigned int index = (obj - slab->mem_ptr) / slab->obj_size;

    ASSERT_PANIC(index <= slab->max_num, "index of free greated than max num entries");
    slab->free_indexs[index] = slab->free_index;
    slab->free_index = index;
    slab->num --;

    return 0;
}

