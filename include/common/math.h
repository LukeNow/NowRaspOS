#ifndef __MATH_H
#define __MATH_H

#include <stddef.h>
#include <stdint.h>

unsigned int math_log2_64(uint64_t num);
unsigned int math_is_power2_64(uint64_t num);
unsigned int math_align_power2_64(uint64_t num);

#define MATH_LOG2(X, RET) do { RET=0; while (num > 1) { num >>= 1; ret++; }} while(0)

#endif