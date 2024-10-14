#include <stddef.h>
#include <stdint.h>
#include <common/common.h>
#include <common/bits.h>
#include <common/assert.h>

unsigned int _bits_count(uint64_t bits, unsigned int num)
{
    unsigned int count = 0;
    uint64_t mask = 1;
    for (int i = 0; i < num; i++) {
        if (bits & mask)
            count ++;
        mask = mask << 1;
    }

    return count;
}

unsigned int bits_msb_index_64(uint64_t bits)
{
    uint64_t mask = 1;

    for (unsigned int i = 0; i < 63; i++) {
        if (bits < mask)
            return i;
        mask = mask << 1;
    }

    return 63;
}

unsigned int bits_msb_index_32(uint32_t bits)
{
    uint64_t mask = 1;

    for (unsigned int i = 0; i < 31; i++) {
        if (bits < mask)
            return i;
        mask = mask << 1;
    }

    return 31;
}

unsigned int bits_count_64(uint64_t bits)
{
    return _bits_count(bits, 64);
}

unsigned int bits_count_32(uint32_t bits)
{
    return _bits_count(bits, 32);
}

void bits_set_64(uint64_t * bits, unsigned int index, unsigned int num_bits)
{
    *bits = *bits | ((uint64_t)BITS(num_bits) << index);
}

void bits_set_32(uint32_t * bits, unsigned int index, unsigned int num_bits)
{
    *bits = *bits | ((uint32_t)BITS(num_bits) << index);
}

uint64_t bits_get_64(uint64_t * bits, unsigned int index, unsigned int num_bits)
{
    return (uint64_t)(*bits & ((uint64_t)BITS(num_bits) << index)) >> index;
}

uint32_t bits_get_32(uint32_t * bits, unsigned int index, unsigned int num_bits)
{
    return (uint32_t)(*bits & ((uint32_t)BITS(num_bits) << index)) >> index;
}

void bits_free_64(uint64_t * bits, unsigned int index, unsigned int num_bits)
{
    *bits = (*bits &~ ((uint64_t)BITS(num_bits) << index));
}

void bits_free_32(uint32_t * bits, unsigned int index, unsigned int num_bits)
{
    *bits = (*bits &~ ((uint32_t)BITS(num_bits) << index));
}

void bits_flip_64(uint64_t * bits, unsigned int index, unsigned int num_bits)
{
    *bits = (*bits ^ ((uint64_t)BITS(num_bits) << index));
}

void bits_flip_32(uint32_t * bits, unsigned int index, unsigned int num_bits)
{
    *bits = (*bits ^ ((uint32_t)BITS(num_bits) << index));
}
