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
void aarch64_mb()
{
    asm volatile ("dmb  ish");
}

/* Data sync barrier
 * Will stall until all writes are completed. */
void aarch64_syncb()
{
    asm volatile ("dsb  ish");
}

/* Flushes instruction prefech cache. */
void aarch64_instrsyncb()
{
    asm volatile ("isb  sy");
}
