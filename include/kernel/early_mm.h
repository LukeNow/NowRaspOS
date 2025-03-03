#ifndef __EARLY_MM_H
#define __EARLY_MM_H

#include <stdint.h>
#include <stddef.h>
#include <kernel/mbox.h>

#define EARLY_MEM_MAP_ENTRY_NUM 4
#define EARLY_MEM_MAP_DEVICE_MEM_START 1

uint64_t early_mmu_get_map_entry(uint64_t addr);
void early_get_mem_size(uint32_t *base_addr, uint32_t *size, mbox_prop_tag_t tag);
void * early_memset(void* bufptr, int value, size_t size);
void * early_memcpy(void * dstptr, void * srcptr, size_t size);
void align_early_mem(size_t size);
uint8_t * early_data_alloc(size_t size);
uint8_t * early_page_data_alloc(unsigned int page_num);
void *mm_earlypage_alloc(int num_pages);
int mm_earlypage_shrink(int num_pages);
int mm_early_is_intialized();
uint8_t * mm_early_get_heap_top();
mmu_mem_map_t * mm_early_get_memmap();
mmu_mem_map_t * mm_early_init_memmap(uint32_t phys_mem_size, uint32_t vc_mem_start, uint32_t vc_mem_size);
int mm_early_init();

#endif