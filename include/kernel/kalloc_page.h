#ifndef __KALLOC_PAGE_H
#define __KALLOC_PAGE_H

#include <common/common.h>
#include <common/linkedlist.h>
#include <kernel/kalloc_cache.h>
#include <kernel/lock.h>
#include <kernel/mm.h>


void kalloc_init_buddy(mm_area_t * area, kalloc_buddy_t * buddy, unsigned int memorder, unsigned int add_to_list);
unsigned int kalloc_get_buddy_page_index(kalloc_buddy_t * buddy);
kalloc_buddy_t * kalloc_get_buddy_from_page_index(unsigned int page_index);
void kalloc_set_buddy_memorder(kalloc_buddy_t * buddy, unsigned int memorder);
kalloc_buddy_t * kalloc_get_buddy_sibling(kalloc_buddy_t * buddy);
kalloc_buddy_t * kalloc_get_buddy_from_addr(uint64_t addr);
int kalloc_page_free_pages(uint64_t addr, flags_t flags);
int kalloc_page_reserve_pages(uint64_t addr, unsigned int memorder, flags_t flags);
uint64_t kalloc_page_alloc_pages(unsigned int memorder, flags_t flags);


#endif