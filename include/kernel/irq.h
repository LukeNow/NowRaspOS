#ifndef __IRQ_H
#define __IRQ_H

void irq_init();
void handle_invalid_irq(int irq_entry);
void handle_irq();
extern void irq_sync_el1(uint64_t vector);
extern void irq_vector_init();
extern void irq_enable();
extern void irq_disable();
extern void irq_save(uint64_t * flags);
extern void irq_save_disable(uint64_t * flags);
extern void irq_restore(uint64_t flags);

#endif