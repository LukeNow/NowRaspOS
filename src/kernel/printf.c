#include <stddef.h>
#include <stdint.h>
#include <kernel/uart.h>
#include <kernel/lock.h>

DEFINE_SPINLOCK(lock);

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

int printf(char *s)
{
    if  (!uart_isready()) 
        return -1;
    
    lock_spinlock(&lock);
    uart_puts(s);

    lock_spinunlock(&lock);
    return 0;
}

int printfdigit(char *s, unsigned int data)
{
     if  (!uart_isready()) 
        return -1;

    printf(s);
    printf(convert(data, 10));
    printf("\n");
}


int printfdata(char *s, uint64_t data)
{
     if  (!uart_isready()) 
        return -1;

    printf(s);
    uart_hex(data);
    printf("\n");

    return 0;   
}

int printfbinary(char *s, uint32_t data)
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