// AArch64 mode
 
// To keep this in the first portion of the binary.
.section ".text.boot"
 
.global _start

.extern early_init
.extern early_core_init

_start:
    // Move the multiboot info to register for later use
    mov x12, x0
    // Save start addr for cores to jump to when started
    adr x11, _start	
 
    ldr     x0, =0x30d00800
    msr     sctlr_el1, x0                         // Update "System Control Register (EL1)":
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
    msr     scr_el3, x0                           // Update "Secure Configuration Register":
                                                  //   set bit 0 => EL0 and EL1 are in non-secure state
                                                  //   set RES:1 bits 4, 5
                                                  //   set bit 10 => EL2 is aarch64, EL2 controls EL1 and EL0 behaviors

    mov     x0, 0x000001c5
    msr     spsr_el3, x0                          // Update "Saved Program Status Register (EL3)":
                                                  //   set bit 0 => dedicated stack pointer selected on EL switch to/from EL3
                                                  //   set bit 2 (and clear bit 3) => drop to EL1 on eret
                                                  //   set bit 6 => mask (disable) error (SError) interrupts
                                                  //   set bit 7 => mask (disable) regular (IRQ) interrupts
                                                  //   set bit 8 => mask (disable) fast (FIQ) interrupts
    ldr     x0, =el1_entry
    msr     elr_el3, x0                           // Update Exception Link Register (EL3):
                                                  //   set to return address after next `eret`
    eret


el2_entry:
    // Since the arm stub boot code puts us in el2 
    // Set stacks in EL2 so that we can set stack ptrs for EL2 and below
    ldr x2, = __EL2_stack_core0				// Address of EL2_stack_core0 stack pointer value
    ldr x3, = __EL1_stack_core0				// Address of EL1_stack_core0 stack pointer value
    ldr x4, = __EL0_stack_core0				// Address of EL0_stack_core0 stack pointer value
	mrs x6, mpidr_el1						// Read core id on ARM8
	ands x6, x6, #0x3						// Make cpu id bitmask
	beq set_stacks							// Ready to set core 0 stacks
    ldr x2, = __EL2_stack_core1				// Address of EL2_stack_core1 stack pointer value
    ldr x3, = __EL1_stack_core1				// Address of EL1_stack_core1 stack pointer value
    ldr x4, = __EL0_stack_core1				// Address of EL0_stack_core1 stack pointer value
	cmp x6, #1								// Check cpu id for core 1
	beq set_stacks							// Ready to set core 2 stacks
    ldr x2, = __EL2_stack_core2				// Address of EL2_stack_core2 stack pointer value
    ldr x3, = __EL1_stack_core2				// Address of EL1_stack_core2 stack pointer value
    ldr x4, = __EL0_stack_core2				// Address of EL0_stack_core2 stack pointer value
	cmp x6, #2								// Check cpu id for core 2
	beq set_stacks							// Ready to set core 3 stacks
    ldr x2, = __EL2_stack_core3				// Address of EL2_stack_core3 stack pointer value
    ldr x3, = __EL1_stack_core3				// Address of EL1_stack_core3 stack pointer value
    ldr x4, = __EL0_stack_core3				// Address of EL0_stack_core3 stack pointer value

set_stacks:
	mov	sp, x2								/* EL2 stack set */
	msr	sp_el1, x3							/* EL1 stack set */
	msr	sp_el0, x4							/* EL0 stack set */

    // Continue to el1 entry
    mov     x0, 0x000001c5
    msr     spsr_el2, x0                          // Update "Saved Program Status Register (EL2)":
                                                  //   set bit 0 => dedicated stack pointer selected on EL switch to/from EL2
                                                  //   set bit 2 (and clear bit 3) => drop to EL1 on eret
                                                  //   set bit 6 => mask (disable) error (SError) interrupts
                                                  //   set bit 7 => mask (disable) regular (IRQ) interrupts
                                                  //   set bit 8 => mask (disable) fast (FIQ) interrupts
    ldr     x0, =el1_entry
    msr     elr_el2, x0                           // Update Exception Link Register (EL2):
                                                  //   set to return address after next `eret`
    eret

el1_entry: 
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbz     x1, master_core

child_core:
    // Child cores will wait for signal that early MMU init is done by main core
    bl early_core_init
    // Move child stack to higher addr
    mov     x1, sp
    mov     x2, #0xffffff8
    lsl     x2, x2, 36
    add     x1, x1, x2
    mov     sp, x1

    // Jump to child main C code
    mrs     x0, mpidr_el1
    and     x0, x0, #3
    bl kernel_child_main

child_hang:
    wfe
    b       child_hang

master_core:
    bl early_init

    // Move stack addr to higher kern address
    mov     x1, sp
    mov     x2, #0xffffff8
    lsl     x2, x2, 36
    add     x1, x1, x2
    mov     sp, x1

    // clear bss
    ldr     x1, =__bss_start
    ldr     w2, =__bss_size
1:  cbz     w2, 2f
    str     xzr, [x1], #8
    sub     w2, w2, #8
    cbnz    w2, 1b

    // jump to C kernel code, should not return    
2:  mov     x0, x12 // multi boot info
    mov     x1, x11 // Core start addr
    mov     x2, #0x0 //res
    mov     x3, #0x0 // res
    bl      kernel_main
   
   // We somehow returned here stop the core
3:
    wfe
    b       3b
