#ifndef __ATOMIC_H
#define __ATOMIC_H

int atomic_cmpxchg(uint64_t *ptr, int old, int new);

#endif