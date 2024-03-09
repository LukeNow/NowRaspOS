#include <stddef.h>
#include <stdint.h>
#include <kernel/irq.h>
#include <kernel/printf.h>
#include <kernel/uart.h>
#include <kernel/sysreg.h>

extern void _irq_init();

void irq_init() 
{

    //*((uint64_t *) ENABLE_IRQS_1) = SYSTEM_TIMER_IRQ_1;
    _irq_init();
}

void handle_invalid_irq(int irq_entry)
{
    printf("IRQ_INVALID_ENTRY: ");
    uart_hex(irq_entry);
    printf("\n");
    return;
}

void handle_irq()
{
    int pending = *((uint64_t*)IRQ_PENDING_1);
    printf("IRQ EL1 FIRED\n");
}