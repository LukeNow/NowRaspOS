#include <stddef.h>
#include <stdint.h>
#include <kernel/mmu.h>
#include <kernel/addr_defs.h>
#include <common/common.h>
#include <kernel/mbox.h>

EARLY_DATA(static int early_page_num);
EARLY_DATA(static uint8_t *early_page_start);
EARLY_DATA(static uint8_t *early_page_end);
EARLY_DATA(static uint8_t *early_page_curr);
EARLY_DATA(static uint8_t *mm_data_top);
EARLY_DATA(static int early_mm_initialized);

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

EARLY_TEXT void* early_memset(void* bufptr, int value, size_t size) 
{
	unsigned char* buf = (unsigned char*) bufptr;
	for (size_t i = 0; i < size; i++)
		buf[i] = (unsigned char) value;
	return bufptr;
}

EARLY_TEXT void align_early_mem(size_t size)
{
    mm_data_top = ALIGN_UP((uint64_t)mm_data_top, size);
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

EARLY_TEXT void *mm_earlypage_alloc(int num_pages)
{
    uint8_t *start = early_page_curr;
    uint8_t *curr = early_page_curr;

    if (start + (PAGE_SIZE * num_pages) > early_page_end) {
        return NULL;
    }

    curr += PAGE_SIZE * num_pages;
    early_page_curr = curr;

    return start;
}

EARLY_TEXT int mm_earlypage_shrink(int num_pages)
{
    uint8_t *start = early_page_curr;
    uint8_t *curr = early_page_curr;

    if ((num_pages * PAGE_SIZE) > start || (start - (PAGE_SIZE * num_pages)) < early_page_start)
        return 1;

    curr -= PAGE_SIZE * num_pages;
    early_page_curr = curr;

    return 0;
}

EARLY_TEXT int mm_early_is_intialized()
{
    return early_mm_initialized;
}

EARLY_TEXT int mm_early_init()
{
    uint32_t early_page_size = ((uint64_t) __earlypage_end) - ((uint64_t) __earlypage_start);
    early_page_num = early_page_size / PAGE_SIZE;
    early_page_start = ((uint64_t) __earlypage_start);
    early_page_end = ((uint64_t) __earlypage_end);
    early_page_curr = early_page_start;

    mm_data_top = (char *)early_page_end;

    //ASSERT(IS_ALIGNED((uint64_t)early_page_start, PAGE_SIZE));
    //ASSERT(IS_ALIGNED((uint64_t)early_page_end, PAGE_SIZE));
    
    early_mm_initialized = 1;

    return 0;
}
