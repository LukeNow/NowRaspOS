#ifndef __KERN_DEFS_H
#define __KERN_DEFS_H

#include <stddef.h>
#include <common/common.h>

#define PAGE_SIZE 4096
#define PAGE_MASK ~(PAGE_SIZE - 1)
#define PAGE_BIT_OFF 12
#define UPPER_ADDR_MASK 0xFFFF000000000000

#define PAGE_INDEX_FROM_PTR(PTR) ((uint64_t)(PTR) / PAGE_SIZE)

#endif