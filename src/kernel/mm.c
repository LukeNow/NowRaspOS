#include <stdint.h>
#include <stddef.h>
#include <common/assert.h>
#include <kernel/addr_defs.h>
#include <kernel/kern_defs.h>
#include <kernel/uart.h>
#include <kernel/lock.h>

DEFINE_SPINLOCK(lock);

static int early_page_num = 0;
static uint8_t *early_page_start;
static uint8_t *early_page_end;
static uint8_t *early_page_curr;

int mm_initialized = 0;

int mm_early_init()
{
    uint32_t early_page_size = ((uint64_t) __earlypage_end) - ((uint64_t) __earlypage_start);
    early_page_num = early_page_size >> PAGE_BIT_OFF;
    early_page_start = ((uint64_t) __earlypage_start);
    early_page_end = ((uint64_t) __earlypage_end);
    early_page_curr = early_page_start;


    ASSERT(IS_ALIGNED((uint64_t)early_page_start, PAGE_SIZE));
    ASSERT(IS_ALIGNED((uint64_t)early_page_end, PAGE_SIZE));

    printf("MM_EARLY_INIT:\n");
    printf("EARLY_PAGE_NUM: ");
    uart_hex(early_page_num);
    printf("\n");
    printf("EARLY_PAGE_START ADDR: ");
    uart_hex(early_page_start);
    printf("\n");
    
    mm_initialized = 1;

    return 0;
}

int mm_init(size_t mem_size, uint64_t *start_addr)
{
    return 0;
}

uint8_t *mm_earlypage_alloc(int num_pages)
{
    ASSERT(mm_initialized);

    lock_spinlock(&lock);

    uint8_t *start = early_page_curr;
    uint8_t *curr = early_page_curr;

    for  (int i = 0; i < num_pages; i++)  {
        if (curr + PAGE_SIZE > early_page_end) {
            ASSERT(0);
            return NULL;
        }
        curr += PAGE_SIZE;
    }

    early_page_curr = curr;
    lock_spinunlock(&lock);

    return start;
}