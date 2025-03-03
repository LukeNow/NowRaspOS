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
    uint8_t core_id = cpu_get_id();
    uint32_t core_irq = QA7->CoreIRQSource[core_id].Raw32;
    unsigned int irq = IRQ->IRQPending1;
    unsigned int pending2 = IRQ->IRQPending2;

    switch (core_irq) {
        case 0:
            break;
        case LOCAL_TIMER_SRC_INT:
            DEBUG("Local timer fired");
            localtimer_clearirq();
            break;
        default:
            DEBUG_DATA("Unknown coreirq pending = ", core_irq);
    }


	switch (irq) {
		case 0:
            break;
        case (SYSTEM_TIMER_IRQ_1):
			//timer_handle_irq();
            systemtimer_clearirq();
            DEBUG("TIMER HIT");
			break;
		default:
			DEBUG_DATA_DIGIT("--Invalid irq fired, irq=", irq);
	}

}