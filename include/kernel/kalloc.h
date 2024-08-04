#ifndef __KALLOC_H
#define __KALLOC_H

#include <stddef.h>
#include <stdint.h>
#include <common/common.h>
#include <kernel/kern_defs.h>
#include <kernel/lock.h>
#include <kernel/kalloc_cache.h>
#include <kernel/kalloc_slab.h>
#include <common/bits.h>

/* We reserve bits from the 16th bit of memory alloc flags to define a standard flag interface. 
 * Other bits are then used for other memory subsystem flags e.g. slab, cache. */
#define KALLOC_FLAG_OFFSET_START 16

int kalloc_init();
void * kalloc_alloc(size_t size, flags_t flags);
int kalloc_free(void * object, flags_t flags);
void * kalloc_pages(unsigned int page_num, flags_t flags);
int kalloc_free_pages(void * page_ptr, unsigned int page_num, flags_t flags);

#endif