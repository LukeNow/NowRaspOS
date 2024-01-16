#include <stddef.h>
#include <stdint.h>
#include <kernel/uart.h>
#include <kernel/lock.h>

DEFINE_SPINLOCK(lock);

int printf(char *s)
{
    if  (!uart_isready()) 
        return -1;
    
    lock_spinlock(&lock);
    uart_puts(s);

    lock_spinunlock(&lock);
    return 0;
}