#ifndef __AARCH64_COMMON
#define __AARCH64_COMMON

#define AARCH64_MSR(REG, VAL) asm volatile ("msr " #REG ", %0" : : "r" (VAL))
#define AARCH64_MRS(REG, VAL) asm volatile ("mrs %0, " #REG : "=r" (VAL))

void aarch64_sev();
void aarch64_wfe();
void aarch64_dmb_inner();
void aarch64_dmb();
void aarch64_dsb_inner();
void aarch64_dsb();
void aarch64_isb();


#endif