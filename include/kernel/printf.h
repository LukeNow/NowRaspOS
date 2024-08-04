#ifndef  __PRINTF_H
#define __PRINTF_H


int printf(const char  *s);
int printfdata(const char *s, uint64_t data);
int printfdigit(const char *s, unsigned int data);
int printfbinary(const char *s, uint32_t data);

#endif