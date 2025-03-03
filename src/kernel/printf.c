#include <stddef.h>
#include <stdint.h>
#include <kernel/uart.h>
#include <common/lock.h>

DEFINE_SPINLOCK(lock);

void lock_printlock()
{
    lock_spinlock(&lock);
}

void unlock_printlock()
{
    lock_spinunlock(&lock);
}

static char *convert(unsigned int num, int base) 
{
	static char representation[] = "0123456789ABCDEF";
	static char buff[50];
	char *ptr;

	ptr = &buff[49];
	*ptr = '\0';

	do {
		*--ptr = representation[num % base];
		num /= base;
	} while(num != 0);
	
		
	return ptr;
}

int printf(const char *s)
{
    if  (!uart_isready()) 
        return -1;
    
    uart_puts(s);

    return 0;
}

int printfdigit(const char *s, unsigned int data)
{
     if  (!uart_isready()) 
        return -1;

    printf(s);
    printf(convert(data, 10));
    printf("\n");
}


int printfdata(const char *s, uint64_t data)
{
     if  (!uart_isready()) 
        return -1;

    printf(s);
    uart_hex(data);
    printf("\n");

    return 0;   
}

int printfbinary(const char *s, uint32_t data)
{
    uint32_t bits = 1 << 31;
    uint32_t d;

    if  (!uart_isready()) 
        return -1;

    printf(s);

    for (int i = 31; i >= 0; i--) {
        bits = 1 << i;
        d = (bits & data) >> i;    
        printf(convert(d, 10));
    }

    printf("\n");
    return 0;

}