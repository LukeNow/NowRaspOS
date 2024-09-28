#include  <stddef.h>
#include  <stdint.h>

extern int _atomic_cmpxchg(uint64_t *ptr, uint64_t old_val, uint64_t new_val);
int atomic_cmpxchg(uint64_t *ptr, uint64_t old_val, uint64_t new_val)
{
	return _atomic_cmpxchg(ptr, old_val, new_val);
}

// For these atomic ops we can do how linux does it
// and dynamically pass instructions we want to be atomic
// since the pattern is the same for these simple ops

extern int _atomic_fetch_add(uint64_t *ptr, uint64_t val);
int atomic_add(uint64_t *ptr, uint64_t val)
{
	return _atomic_fetch_add(ptr, val);
}

extern int _atomic_fetch_sub(uint64_t *ptr, uint64_t val);
int atomic_sub(uint64_t *ptr, uint64_t val)
{
	return _atomic_fetch_sub(ptr, val);
}

extern int _atomic_fetch_or(uint64_t *ptr, uint64_t val);
int atomic_or(uint64_t *ptr, uint64_t val)
{
	return _atomic_fetch_or(ptr, val);
}