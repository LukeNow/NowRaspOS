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
#include <kernel/task.h>
#include <kernel/mbox.h>
#include <kernel/sched.h>
#include <emb-stdio/emb-stdio.h>

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
    cpu_core_dump_all();

    while (1) {}
}

void localtimer_handle()
{
    localtimer_clearirq();
    localtimer_isr_tick();
    sched_timer_isr();
}

void handle_irq()
{   
    uint64_t flags;
    uint32_t core_irq;
    uint8_t core_id;
    uint32_t irq;
    irq_disable();

    core_id = cpu_get_id();
    irq = IRQ->IRQPending1;
    //unsigned int pending2 = IRQ->IRQPending2;

	switch (irq) {
		case 0:
            break;
        case (SYSTEM_TIMER_IRQ_1):
            systemtimer_clearirq();
			break;
		default:
			DEBUG_DATA_DIGIT("--Invalid irq fired, irq=", irq);
	}

    core_irq = QA7->CoreIRQSource[core_id].Raw32;
    
    //TODO MAKE THIS MORE EFFICENT
    for (int i = 0; i < 32; i++) {
        uint32_t core_irq_mask = core_irq & (1 << i);
        switch (core_irq_mask) {
            case LOCAL_TIMER_SRC_INT:
                localtimer_handle();
                core_irq &= ~(LOCAL_TIMER_SRC_INT);
                break;
            case LOCAL_MBOX_SRC_INT(0):
                mbox_handle_core_int(core_id, 0);

                core_irq &= ~LOCAL_MBOX_SRC_INT(0);
                break;
            case LOCAL_MBOX_SRC_INT(1):
                mbox_handle_core_int(core_id, 1);

                core_irq &= ~LOCAL_MBOX_SRC_INT(1);
                break;
            case LOCAL_MBOX_SRC_INT(2):
                mbox_handle_core_int(core_id, 2);

                core_irq &= ~LOCAL_MBOX_SRC_INT(2);
                break;
            case LOCAL_MBOX_SRC_INT(3):

                core_irq &= ~LOCAL_MBOX_SRC_INT(3);
                mbox_handle_core_int(core_id, 3);
                break;
        }
    }

    if (core_irq) {
        DEBUG_DATA("Unhandled irq=", core_irq);
        DEBUG_PANIC_ALL("Unhandled irq");
    }

    irq_enable();
}