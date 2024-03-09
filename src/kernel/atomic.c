#include  <stddef.h>
#include  <stdint.h>


extern int _atomic_cmpxchg(uint64_t *ptr, int old, int new);
int atomic_cmpxchg(uint64_t *ptr, int old, int new)
{
	return _atomic_cmpxchg(ptr, old, new);
}

// For these atomic ops we can do how linux does it
// and dynamically pass instructions we want to be atomic
// since the pattern is the same for these simple ops

extern int _atomic_fetch_add(uint64_t *ptr, int val);
int atomic_add(uint64_t *ptr, int val)
{
	return _atomic_fetch_add(ptr, val);
}

extern int _atomic_fetch_or(uint64_t *ptr, int val);
int atomic_or(uint64_t *ptr, int val)
{
	return _atomic_fetch_or(ptr, val);
}