#include <stddef.h>
#include <stdint.h>
#include <common/common.h>
#include <common/bits.h>

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

unsigned int bits_count_64(uint64_t bits)
{
    return _bits_count(bits, 64);
}

unsigned int bits_count_32(uint32_t bits)
{
    return _bits_count(bits, 32);
}