#include <stddef.h>
#include <stdint.h>
#include <common/common.h>
#include <kernel/timer.h>
#include <kernel/irq.h>

static const unsigned int timer_interval = 200000;
static unsigned int curr_time = 0;

unsigned int timer_get_time()
{
    return curr_time;
}

void timer_init()
{
	curr_time = GET32(TIMER_CLO);
	curr_time += timer_interval;
	PUT32(TIMER_C1, curr_time);
}

void timer_enable()
{
    irq_disable();
    PUT32(ENABLE_IRQS_1, SYSTEM_TIMER_IRQ_1);
    irq_enable();
}

void timer_handle_irq()
{
    curr_time += timer_interval;
    PUT32(TIMER_C1, curr_time);
    PUT32(TIMER_CS, TIMER_CS_M1);
}
