#ifndef __UART_H
#define __UART_H

int uart_isready();
void uart_init();
void uart_send(unsigned int c);
char uart_getc();
void uart_puts(char *s);
void uart_hex(uint64_t d);

#endif