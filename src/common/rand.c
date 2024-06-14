#include <stddef.h>
#include <stdint.h>
#include <common/rand.h>

static uint32_t next = 1; 
 
// C standard psuedo random generator, simple enough for our uses for now
int rand_prng(void)  // RAND_MAX assumed to be 32767
{
    next = next * 1103515245 + 12345;
    return (unsigned int) (next / 65536) % RAND_PRNG_MAX;
}
 
void rand_prng_seed(unsigned int seed)
{
    next = seed;
}