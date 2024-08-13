// AArch64 mode
 
// To keep this in the first portion of the binary.
.section ".text.boot"
 
// Make _start global.
.global _start

// 
_start:

    mov x12, x0
     // read cpu id, stop slave cores
    mrs     x1, mpidr_el1
    and     x1, x1, #3

    cbz     x1, master

child_entry:  // Child core enter
    ldr     x3, =__stackpage_start
    mov     x4, #4096
    // x3 = __stack_page_start + 4k
    add     x3, x3, x4     
    // x2 = (__stack_page_start + 4k) + (corenum * 4k)
    madd    x2,  x4, x1, x3 
    mov sp, x2

    mrs     x0, mpidr_el1
    bl kernel_child_main
1:
    // stop core
    wfe
    b 1b      

master:
    //ldr	x0, =SCTLR_VALUE_MMU_DISABLED
	//msr	sctlr_el1, x0		

	//ldr	x0, =HCR_VALUE
	//msr	hcr_el2, x0

	//ldr	x0, =SCR_VALUE
	//msr	scr_el3, x0
	//ldr	x0, =SPSR_VALUE
	//msr	spsr_el3, x0
	//adr	x0, el1_entry		
	//msr	elr_el3, x0
	//eret


    ldr     x0, =0x30d00800
    msr     sctlr_el1, x0                           // Update "System Control Register (EL1)":
                                                  //   set RES:1 bits (11, 20, 22, 23, 28, 29)
                                                  //   disable caches (bits 2, 12)
    mov     x0, 0x80000000
    msr     hcr_el2, x0   

    mrs x0, CurrentEL
    and x0, x0, #0x0c
    cmp x0, #0x0c
    bne el2_entry

el3_entry:
    mov     x0, 0x00000431
    msr     scr_el3, x0                             // Update "Secure Configuration Register":
                                                  //   set bit 0 => EL0 and EL1 are in non-secure state
                                                  //   set RES:1 bits 4, 5
                                                  //   set bit 10 => EL2 is aarch64, EL2 controls EL1 and EL0 behaviors

    mov     x0, 0x000001c5
    msr     spsr_el3, x0                            // Update "Saved Program Status Register (EL3)":
                                                  //   set bit 0 => dedicated stack pointer selected on EL switch to/from EL3
                                                  //   set bit 2 (and clear bit 3) => drop to EL1 on eret
                                                  //   set bit 6 => mask (disable) error (SError) interrupts
                                                  //   set bit 7 => mask (disable) regular (IRQ) interrupts
                                                  //   set bit 8 => mask (disable) fast (FIQ) interrupts
    ldr     x0, =el1_entry
    msr     elr_el3, x0                             // Update Exception Link Register (EL3):
                                                  //   set to return address after next `eret`
    eret


el2_entry:
    mov     x0, 0x000001c5
    msr     spsr_el2, x0                            // Update "Saved Program Status Register (EL2)":
                                                  //   set bit 0 => dedicated stack pointer selected on EL switch to/from EL2
                                                  //   set bit 2 (and clear bit 3) => drop to EL1 on eret
                                                  //   set bit 6 => mask (disable) error (SError) interrupts
                                                  //   set bit 7 => mask (disable) regular (IRQ) interrupts
                                                  //   set bit 8 => mask (disable) fast (FIQ) interrupts
    ldr     x0, =el1_entry
    msr     elr_el2, x0                             // Update Exception Link Register (EL2):
                                                  //   set to return address after next `eret`
    eret



el1_entry:  // cpu id == 0
    // set top of stack at __stackpage_start + 4k page

    ldr     x1, = __stackpage_start
    add     x1, x1, #4096
    mov     sp, x1
    
    // clear bss
    ldr     x1, =__bss_start
    ldr     w2, =__bss_size
3:  cbz     w2, 4f
    str     xzr, [x1], #8
    sub     w2, w2, #8
    cbnz    w2, 3b

    // jump to C kernel code, should not return    
4:  mov     x0, x12
    mov     x1, #0x0
    mov     x2, #0x0
    mov     x3, #0x0
    bl      kernel_main
   
   // stop core
5:
    wfe
    b       5b
