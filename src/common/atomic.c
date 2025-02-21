#include  <stddef.h>
#include  <stdint.h>

int atomic_cmpxchg_try_64(uint64_t * ptr, uint64_t old_val, uint64_t new_val, uint32_t tries)
{
	int ret;

	while (tries) {
		ret = atomic_cmpxchg_64(ptr, old_val, new_val);
		if (!ret)
			return ret;
		
		tries--;
	}

	return ret;
}
