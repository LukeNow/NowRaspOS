#ifndef __COMMON_H
#define __COMMON_H

#define CYCLE_WAIT(x) do { int i = (x); while (i--) { asm volatile ("nop"); } } while (0)

#endif