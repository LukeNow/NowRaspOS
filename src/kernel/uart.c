#include <stdint.h>
#include <stddef.h>

#include <kernel/gpio.h>
#include <common/common.h>
#include <kernel/aarch64_common.h>

static int uart_ready = 0;

int uart_isready()
{
    return uart_ready;
}

void uart_init()
{
    register unsigned int reg;

    *AUX_ENABLE |= 1;
    *AUX_MU_CNTL = 0;
    *AUX_MU_LCR = 3;
    *AUX_MU_MCR = 0;
    *AUX_MU_IER = 0;
    *AUX_MU_IIR = 0xc6;
    *AUX_MU_BAUD = 270;

    reg = *GPFSEL1;
    reg &= ~((7 << 12) | (7 << 15)); // Clear the gpio 14,15 pins function sel
    reg |= (2 << 12) | (2 << 15); // set gpio14,15 to alt 5 mode (uart1 tx/rx)
    *GPFSEL1 = reg;

    *GPPUD = 0; // Disable pullup pulldown to enable the pins on clock assert
    CYCLE_WAIT(300); // Min wait 150 cycles but double to make sure
    *GPPUDCLK0 = (1 << 14) | (1 << 15); // Program the clock assert to program the pins
    CYCLE_WAIT(300);
    *GPPUDCLK0 = 0; // Now disable the clk assert on the pins that they are programed
    *AUX_MU_CNTL = 3; // Finally enable miniuart TX/RX

    aarch64_dsb();
    uart_ready = 1;
}

void uart_send(unsigned int c) {
    // Wait until we are able to send
    while (!(*AUX_MU_LSR & 0x20)) {
        CYCLE_WAIT(1);
    }

    *AUX_MU_IO = c;
}

void uart_puts(char *s) {
    while (*s != '\0') {
        if (*s == '\n')
            uart_send('\r');
        uart_send(*s++);
    }
}


char uart_getc() {
    char reg;

    // Wait until we are able to receive
    while (!(*AUX_MU_LSR & 0x01)) {
        CYCLE_WAIT(1);
    }

    reg = (char)(*AUX_MU_IO);
    reg = (reg == '\r') ? '\n' : reg;
    return reg;
}

void uart_hex(uint64_t d) {
    uint32_t n;
    uart_puts("0x");
    for(int c = 60; c >= 0; c-=4) {
        // get highest tetrad
        n = (uint32_t)((d >> c) & 0xF);
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        n += n > 9 ? 0x37 : 0x30;
        uart_send(n);
    }
}