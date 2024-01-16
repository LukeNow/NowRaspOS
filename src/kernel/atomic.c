#include  <stddef.h>
#include  <stdint.h>

extern int _atomic_cmpxchg(uint64_t *ptr, int old, int new, int *currval);
int atomic_cmpxchg(uint64_t *ptr, int old, int new)
{
	int currval;
	unsigned long res;

	do {
		res = _atomic_cmpxchg(ptr, old, new, &currval);
	} while (res);

	return currval;
}