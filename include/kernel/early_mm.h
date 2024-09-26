#ifndef __EARLY_MM_H
#define __EARLY_MM_H

#include <stdint.h>
#include <stddef.h>
#include <kernel/mbox.h>

void early_get_mem_size(uint32_t *base_addr, uint32_t *size, mbox_prop_tag_t tag);
void* early_memset(void* bufptr, int value, size_t size);
void align_early_mem(size_t size);
uint8_t * early_data_alloc(size_t size);
uint8_t * early_page_data_alloc(unsigned int page_num);
void *mm_earlypage_alloc(int num_pages);
int mm_earlypage_shrink(int num_pages);
int mm_early_is_intialized();
int mm_early_init();

#endif