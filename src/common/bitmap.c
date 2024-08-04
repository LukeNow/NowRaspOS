#include <stddef.h>
#include <stdint.h>
#include <common/assert.h>
#include <common/bits.h>
#include <common/bitmap.h>

#define BITS_PER_BITMAP 64

inline unsigned int _bitmap_index(unsigned int index)
{
    return index / BITS_PER_BITMAP;
}

inline unsigned int _bit_index(unsigned index)
{
    return index % BITS_PER_BITMAP;
}

void bitmap_set(bitmap_t * bitmap, unsigned int index) {

   bits_set_64(&bitmap[_bitmap_index(index)], _bit_index(index), 1);
}

unsigned int bitmap_get(bitmap_t * bitmap, unsigned int index)
{
    return bits_get_64(&bitmap[_bitmap_index(index)], _bit_index(index), 1);
}

void bitmap_flip(bitmap_t * bitmap, unsigned int index)
{
    bits_flip_64(&bitmap[_bitmap_index(index)], _bit_index(index), 1);
}

void bitmap_free(bitmap_t * bitmap, unsigned int index)
{
    bits_free_64(&bitmap[_bitmap_index(index)], _bit_index(index), 1);
}
