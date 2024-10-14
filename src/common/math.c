#include <stddef.h>
#include <stdint.h>
#include <common/math.h>
#include <kernel/printf.h>
#include <kernel/uart.h>
#include <common/assert.h>
#include <common/bits.h>

// NUM cannot be 0
unsigned int math_log2_64(uint64_t num)
{
    ASSERT(num);
    int ret = 0;

    while (num > 1) {
        num >>= 1;
        ret++;
    }

    return ret;
}

// 1 for true, 0 for false. Can be confusing when posix definitions for errors are non zero :)
unsigned int math_is_power2_64(uint64_t num)
{
    uint64_t mask = 1;

    for (int i = 0; i < 64; i++) {
        // We found a 2^n bit
        if (num & mask) {
            // There are other digits so this cannot be power of 2
            if (num & ~mask)
                return 0;
            // There are no other digits so this is a power of 2
            else
                return 1;
        }

        mask <<= 1;
    }

    // We shouldnt reach here
    ASSERT(0);
    return 1;
}

unsigned int math_align_power2_64(uint64_t num)
{
    unsigned int msb = bits_msb_index_64(num);

    return 1 << msb;
}