#ifndef __BITMAP_H
#define __BITMAP_H

#include <stddef.h>
#include <stdint.h>
#include <common/common.h>
#include <common/string.h>

#define BITMAP_BITS_PER_BITMAP_ENTRY 64

typedef uint64_t bitmap_t;

void bitmap_set(bitmap_t * bitmap, unsigned int index);
void bitmap_flip(bitmap_t * bitmap, unsigned int index);
unsigned int bitmap_get(bitmap_t * bitmap, unsigned int index);
void bitmap_free(bitmap_t * bitmap, unsigned int index);

#endif
