#ifndef __IRQ_H
#define __IRQ_H

#include <kernel/sysreg.h>

void irq_init();
void handle_invalid_irq(int irq_entry);
void handle_irq();

#endif