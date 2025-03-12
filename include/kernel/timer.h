#ifndef __KERN_TIMER
#define __KERN_TIMER

typedef uint64_t ticks_t;

uint64_t timer_difference(uint64_t time1, uint64_t time2);
uint64_t systemtimer_gettime_64();
void systemtimer_wait(uint64_t usec);
void systemtimer_clearirq();
void systemtimer_initirq(uint32_t usec);
void generictimer_init(uint32_t msec);

/* The ARM timer seems not to be availabe in QEMU, for virtualization we will use the
 * systemtimer and the percpu local timer. */
void armtimer_clearirq();
void armtimer_init(uint32_t period_in_us);
void armtimer_irq_init(uint32_t period_in_us);

uint64_t localtimer_gettime();
void localtimer_isr_tick();
void localtimer_clearirq();
void localtimer_init(uint32_t period_in_us);
void localtimer_irqinit(uint32_t period_in_us, uint8_t corenum);

#endif