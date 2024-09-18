#include <stddef.h>
#include <stdint.h>

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
