#include <stddef.h>
#include <stdint.h>
#include <common/common.h>
#include <kernel/timer.h>
#include <kernel/irq.h>
#include <common/aarch64_common.h>
#include <kernel/cpu.h>
#include <kernel/mbox.h>
#include <common/assert.h>

#define LOCAL_TIMER_SCALAR 384  // 2 * 19.2Mhz crystal freq = 38.4Mhz
#define ARM_PRESCALAR_DIV 250

uint32_t systemtimer_reload_usec = 20000;

uint64_t timer_difference(uint64_t time1, uint64_t time2)
{
    /* Assuming time1 was taken before time2, we can assume the timer rolled and we can calculate 
       the difference to get the appropriate time. */
    if (time1 > time2) {
        uint64_t diff = UINT64_MAX - time1 + 1;
        return time2 + diff;
    }

    return time2 - time1;
}

/* The System timer runs at 1Mhz. We do two 32 bit reads while making sure that the High count has not changed. */
uint64_t systemtimer_gettime_64()
{
    uint32_t high;
    uint32_t low;

    do {
        high = SYSTEMTIMER->TimerHi;
        low = SYSTEMTIMER->TimerLo;
    } while (high != SYSTEMTIMER->TimerHi);

    return (((uint64_t)high << 32) | low);
}

/* Do a NO-OP wait while we wait for the system timer to catchup to our desired time value. */
void systemtimer_wait(uint64_t usec)
{
    usec += systemtimer_gettime_64();
    while (systemtimer_gettime_64() < usec) {};
}

void systemtimer_clearirq()
{   
    /* Overflow is OKAY since it will overflow and trigger on the new value of the low register. */
    SYSTEMTIMER->Compare1 = SYSTEMTIMER->TimerLo + systemtimer_reload_usec;
    SYSTEMTIMER->ControlStatus = 1 << 1;
}

void systemtimer_initirq(uint32_t usec)
{
    uint32_t low;
    uint32_t mask;
    systemtimer_reload_usec = usec;

    systemtimer_clearirq();
    mask = IRQ->EnableIRQs1 | SYSTEM_TIMER_IRQ_1;
    IRQ->EnableIRQs1 = mask;
}

void generictimer_init(uint32_t msec)
{
    uint32_t freq;
    uint32_t ticks;
    asm volatile ("mrs %0, CNTFRQ_EL0" : "=r" (freq));
    
    ticks = (freq / 1000) * msec;

    asm volatile ("msr CNTP_TVAL_EL0, %0" : : "r" (ticks)); // Load timer

    asm volatile ("msr CNTP_CTL_EL0, %0" : : "r" (1)); // Enable timer


    DEBUG_DATA_DIGIT("Freq=", freq);
}


void armtimer_clearirq()
{
    ARMTIMER->Clear = 1;
}

void armtimer_init(uint32_t period_in_us)
{
    uint32_t buffer[5];
    uint32_t divisor;
    int res = 0;
   
    ARMTIMER->Control.TimerEnable = 0;

    // Get the GPU clock speed, the Armtimer prescalar is set to 250
    res = mbox_request(&buffer[0], 5, MAILBOX_TAG_GET_CLOCK_RATE, 8, 8, CLK_CORE_ID, 0);

    if (res) {
        DEBUG_PANIC("Arm timer init failed at mbox clock request");
    }
    // Set the divisor to get from the range of Hz to uSeconds
    divisor = buffer[4] / ARM_PRESCALAR_DIV;
    divisor /= 10000;
    divisor *= period_in_us;
    divisor /= 100;
    
    DEBUG_DATA_DIGIT("Divisor=", divisor);
    if (!divisor) {
        DEBUG_PANIC("Clock divisor is 0");
    }

    aarch64_dmb();

    ARMTIMER->Load = divisor;
    ARMTIMER->Reload = divisor;
    ARMTIMER->Control.Counter32Bit = 1;
    ARMTIMER->Control.Prescale = Clkdiv1;
    ARMTIMER->Control.TimerEnable = 1;
}

void armtimer_irq_init(uint32_t period_in_us)
{
    armtimer_init(period_in_us);
    
    IRQ->EnableBasicIRQs.Enable_Timer_IRQ = 1;
    ARMTIMER->Control.TimerIrqEnable = 1;
    armtimer_clearirq();
}

void localtimer_clearirq()
{
    QA7->TimerClearReload.IntClear = 1;
    QA7->TimerClearReload.Reload = 1;
}

void localtimer_init(uint32_t period_in_us)
{
    uint32_t divisor = LOCAL_TIMER_SCALAR * period_in_us;
    divisor /= 10;

    QA7->TimerControlStatus.ReloadValue = divisor;
    QA7->TimerControlStatus.TimerEnable = 1;
    QA7->TimerClearReload.Reload = 1;
}

void localtimer_irqinit(uint32_t period_in_us, uint8_t corenum)
{
    localtimer_init(period_in_us);

    QA7->TimerRouting.Routing = LOCALTIMER_TO_CORE0_IRQ + corenum;
    QA7->TimerControlStatus.IntEnable = 1;
    QA7->TimerClearReload.IntClear = 1;
    QA7->TimerClearReload.Reload = 1;
    QA7->CoreTimerIntControl[corenum].nCNTPNSIRQ_IRQ = 1;
    QA7->CoreTimerIntControl[corenum].nCNTPNSIRQ_FIQ = 0;
}
