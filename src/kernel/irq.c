#include <stddef.h>
#include <stdint.h>
#include <kernel/irq.h>
#include <kernel/printf.h>
#include <kernel/uart.h>
#include <kernel/gpio.h>
#include <common/assert.h>
#include <common/common.h>
#include <kernel/timer.h>
#include <common/aarch64_common.h>
#include <common/bits.h>
#include <kernel/cpu.h>

void irq_init() 
{
    DEBUG("--- IRQ INIT START ---");

    irq_vector_init();
    irq_enable();

    DEBUG("--- IRQ INIT DONE---");
}

void handle_invalid_irq(int irq_entry)
{
    printf("IRQ_INVALID_ENTRY: ");
    uart_hex(irq_entry);
    printf("\n");
    return;
}

void handle_sync_irq(uint64_t exception)
{
    uint64_t esr;
    DEBUG_DATA("SYNC HANDLER CALLED =", exception);

    DEBUG_DATA("CPU ID=", cpu_get_id());
    AARCH64_MRS(esr_el1, esr);

    DEBUG_DATA("Code1=", esr);
    DEBUG_DATA_BINARY("Code1=", (uint32_t)esr);
    DEBUG_DATA_BINARY("Code2=", (uint32_t)esr>>32);

    DEBUG_PANIC("STOP");
}

void handle_irq()
{   
    unsigned int irq = GET32(IRQ_PENDING_1);
	switch (irq) {
		case (SYSTEM_TIMER_IRQ_1):
			timer_handle_irq();
			break;
		default:
			DEBUG_DATA_DIGIT("--Invalid irq fired, irq=", irq);
	}
}