#include <stddef.h>
#include <stdint.h>
#include <kernel/mmu.h>
#include <kernel/addr_defs.h>
#include <common/common.h>
#include <kernel/mbox.h>
#include <kernel/early_mm.h>
#include <kernel/mmu.h>

EARLY_DATA(static unsigned int early_page_num);
EARLY_DATA(static uint8_t *early_page_start);
EARLY_DATA(static uint8_t *early_page_end);
EARLY_DATA(static uint8_t *early_page_curr);
EARLY_DATA(static uint8_t *mm_data_top);
EARLY_DATA(static int early_mm_initialized);
EARLY_DATA(static mmu_mem_map_t * phys_mem_map);

/* Hardcoded mbox call to get the memory sizes for phys mem space and 
 * videocore/device memory. 
 * Same thing with the memory map, these values are pretty hard coded but fetch them
 * to properly inspect the memory values for sanities sake. */
EARLY_TEXT void early_get_mem_size(uint32_t *base_addr, uint32_t *size, mbox_prop_tag_t tag)
{
    uint32_t  __attribute__((aligned(16))) mbox[36];
    mbox_message_t msg;

    mbox[0] = 8*4;                  // length of the message
    mbox[1] = MBOX_REQUEST;         // this is a request message
    mbox[2] = tag;   
    mbox[3] = 8;                    // buffer size
    mbox[4] = 8;
    mbox[5] = 0;                    // clear output buffer
    mbox[6] = 0;
    mbox[7] = MBOX_TAG_LAST;
    msg = (mbox_message_t) (((uint64_t)&mbox[0] & ~0xF) | (MBOX_CH_PROP & 0xF));
    while (*MBOX_STATUS & MBOX_FULL) {
        CYCLE_WAIT(3);
    }

    *MBOX_WRITE = (*(uint32_t *)&msg);
    *base_addr = 0;
    *size = 0;

    while (1) {
        while (*MBOX_STATUS & MBOX_EMPTY) {
            CYCLE_WAIT(3);
        }
        *(uint32_t *)&msg = *MBOX_READ;
        if (*(uint32_t *)&msg != 0 && mbox[1] == MBOX_RESPONSE) {
            *base_addr = mbox[5];
            *size = mbox[6];
            break;
	    }
    }
}

/* Early memset/memcpy variants that will live in the lower mem space
 * for the early init environment. */
EARLY_TEXT void * early_memset(void * bufptr, int value, size_t size) 
{
    unsigned char * buf = (unsigned char*) bufptr;
	for (size_t i = 0; i < size; i++)
		buf[i] = (unsigned char) value;
	return bufptr;
}

EARLY_TEXT void * early_memcpy(void * dstptr, void * srcptr, size_t size) 
{
	unsigned char * dst = (unsigned char*) dstptr;
	unsigned char * src = (const unsigned char*) srcptr;
	for (size_t i = 0; i < size; i++)
		dst[i] = src[i];
	return dstptr;
}

/* Early data heap allocators that will allocate AFTER the early pages.
 * We will keep track of this heap in our proper memory allocators
 * but probably never free it since it will have all of our early data. */
EARLY_TEXT void align_early_mem(size_t size)
{
    mm_data_top = (uint8_t*)ALIGN_UP((uint64_t)mm_data_top, size);
}

EARLY_TEXT uint8_t * early_data_alloc(size_t size)
{   
    uint8_t * top = mm_data_top;
    mm_data_top += size;
    align_early_mem(sizeof(uint64_t));
    early_memset(top, 0, size);

    return top;
}

EARLY_TEXT uint8_t * early_page_data_alloc(unsigned int page_num)
{
    align_early_mem(PAGE_SIZE);
    return early_data_alloc(page_num * PAGE_SIZE);
}

/* Early page allocator. */
EARLY_TEXT void * mm_earlypage_alloc(int num_pages)
{
    uint8_t *start = early_page_curr;

    if (start + (PAGE_SIZE * num_pages) > early_page_end) {
        return NULL;
    }

    early_page_curr += PAGE_SIZE * num_pages;

    return start;
}

EARLY_TEXT int mm_earlypage_shrink(int num_pages)
{
    // Account for underflow
    if ((num_pages * PAGE_SIZE) > early_page_curr || (early_page_curr - (PAGE_SIZE * num_pages)) < early_page_start)
        return 1;

    early_page_curr -= PAGE_SIZE * num_pages;

    return 0;
}

EARLY_TEXT uint8_t * mm_early_get_heap_top()
{
    return mm_data_top;
}

EARLY_TEXT int mm_early_is_intialized()
{
    return early_mm_initialized;
}

EARLY_TEXT mmu_mem_map_t * mm_early_get_memmap()
{
    return phys_mem_map;
}

/* Calculate and store the early physical memory map for initial paging setup 
 * and memory size info for allocators. 
 *
 * TODO make this mem map more visible somewhere, since this mem map is pretty hardcoded
 * on rasbi3b+ but I like having the values calculated for sanities sake. */
EARLY_TEXT mmu_mem_map_t * mm_early_init_memmap(uint32_t phys_mem_size, uint32_t vc_mem_start, uint32_t vc_mem_size)
{   
    uint64_t mmio_ram_size = vc_mem_size - MMIO_BASE;
    uint64_t vc_ram_size = MMIO_BASE - vc_mem_start;

    mmu_mem_map_t map[] = {
        {0, vc_mem_start, PT_MEM_ATTR(MT_NORMAL) | PT_INNER_SHAREABLE}, // Normal ram memory
        {vc_mem_start, vc_ram_size, PT_MEM_ATTR(MT_NORMAL_NC)}, // Video core ram, we dont want to cache these accesses
        {MMIO_BASE, mmio_ram_size, PT_MEM_ATTR(MT_DEVICE_NGNRNE)}, // MMIO access to registers and other peripherals
        {0, 0, 0}
    };

    phys_mem_map = (mmu_mem_map_t *)mm_earlypage_alloc(1);
    early_memset(phys_mem_map, 0, PAGE_SIZE);
    early_memcpy(phys_mem_map, &map[0], sizeof(mmu_mem_map_t) * EARLY_MEM_MAP_ENTRY_NUM);

    return phys_mem_map;
}

EARLY_TEXT int mm_early_init()
{
    uint32_t early_page_size = ((uint64_t) __earlypage_end) - ((uint64_t) __earlypage_start);
    early_page_num = early_page_size / PAGE_SIZE;
    early_page_start = (uint8_t*)((uint64_t) __earlypage_start);
    early_page_end = (uint8_t*)((uint64_t) __earlypage_end);
    early_page_curr = early_page_start;

    mm_data_top = (char *)early_page_end;

    early_mm_initialized = 1;

    return 0;
}
