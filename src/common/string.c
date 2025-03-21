#include <stdint.h>
#include <stddef.h>
#include <common/string.h>


void* memmove(void* dstptr, const void* srcptr, size_t size) 
{
	unsigned char* dst = (unsigned char*) dstptr;
	const unsigned char* src = (const unsigned char*) srcptr;
	if (dst < src) {
		for (size_t i = 0; i < size; i++)
			dst[i] = src[i];
	} else {
		for (size_t i = size; i != 0; i--)
			dst[i-1] = src[i-1];
	}
	return dstptr;
}

int memcmp(const void* aptr, const void* bptr, size_t size) 
{
	const unsigned char* a = (const unsigned char*) aptr;
	const unsigned char* b = (const unsigned char*) bptr;
	for (size_t i = 0; i < size; i++) {
		if (a[i] < b[i])
			return -1;
		else if (b[i] < a[i])
			return 1;
	}
	return 0;
}

void* memset(void* bufptr, int value, size_t size) 
{
	unsigned char* buf = (unsigned char*) bufptr;
	for (size_t i = 0; i < size; i++)
		buf[i] = (unsigned char) value;
	return bufptr;
}

void *memset_64(uint64_t * bufptr, uint64_t val, size_t size)
{	
	uint64_t * bufend;

    size += (size % sizeof(uint64_t)); //Round up to nearest uint64_t
	bufend = (uint64_t*)((uint8_t *)bufptr + size);

	for (uint64_t *buf = bufptr; buf < bufend; buf++) {
		*buf = val;
	}

	return bufptr;
}

void* memcpy(void *restrict dstptr, const void *restrict srcptr, size_t size) 
{
	unsigned char* dst = (unsigned char*) dstptr;
	const unsigned char* src = (const unsigned char*) srcptr;
	for (size_t i = 0; i < size; i++)
		dst[i] = src[i];
	return dstptr;
}

size_t strlen(const char *str) 
{
	size_t len = 0;
	while (str[len])
		len++;
	return len;
}

int strcmp(const char *str1, const char *str2)
{
	size_t len1 = strlen(str1);
	size_t len2 = strlen(str2);
	size_t cmp_len = (len1 < len2) ? len1 : len2;
	
	return memcmp(str1, str2, cmp_len + 1);
}

int strncmp(const char *str1, const char *str2, size_t n)
{
	register unsigned char u1, u2;

	while (n-- > 0) {
		u1 = (unsigned char) *str1++;
		u2 = (unsigned char) *str2++;
		if (u1 != u2)
			return u1 - u2;
		if (u1 == '\0')
			return 0;
	}
	return 0;	
}