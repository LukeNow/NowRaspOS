#ifndef __RAND_H
#define __RAND_H


#define RAND_PRNG_MAX 32767

void rand_prng_seed(unsigned int seed);
int rand_prng(void);



#endif