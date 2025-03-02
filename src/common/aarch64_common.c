#include <stddef.h>
#include <stdint.h>

void aarch64_nop()
{
    asm volatile ("nop");
}

/* This is a naked svc call, the immediate is ignored in the handler
 * Right now we store the parameter through a register, this is how Linux does it I believe.
 * We will provide the interface through irq.c and maybe tie it back here for general 
 * calls. 
 */
void aarch64_svc()
{
    asm volatile ("svc #0x10");
}

/* Wait for event
 * Puts into low power mode until woken up by event. */
void aarch64_wfe()
{
    asm volatile ("wfe");
}

void aarch64_sev()
{
    asm volatile ("sev");
}

/* Full memory barrier. 
 * Does not stall execution. */
void aarch64_dmb_inner()
{
    asm volatile ("dmb  ish");
}

void aarch64_dmb()
{
    asm volatile ("dmb  sy");
}

/* Data sync barrier
 * Will stall until all writes are completed. */
void aarch64_dsb_inner()
{
    asm volatile ("dsb  ish");
}

void aarch64_dsb()
{
    asm volatile ("dsb  sy");
}

/* Flushes instruction prefech cache for full system. */

void aarch64_isb()
{
    asm volatile ("isb");
}

void aarch64_cache_flush_invalidate(uint64_t addr)
{
    asm volatile ("dc civac, %0" : : "r" (addr) : "memory");
    // Make sure to put a memory barrier to make sure that the invalidate is finished before continuing
    aarch64_dsb();
}
