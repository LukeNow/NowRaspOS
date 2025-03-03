.extern uart_hex

.macro handle_invalid_entry type
  kernel_entry
  mov x0, #\type
  bl handle_invalid_irq
  mrs x0, esr_el1
  bl uart_hex
  mrs x0, elr_el1
  bl uart_hex

  1: 
    b 1b
.endm

.macro ventry label
    .align 7
    b       \label
.endm


.macro kernel_entry
  sub     sp, sp, #16 * 16
  stp     x0, x1, [sp, #16 * 0]
  stp     x2, x3, [sp, #16 * 1]
  stp     x4, x5, [sp, #16 * 2]
  stp     x6, x7, [sp, #16 * 3]
  stp     x8, x9, [sp, #16 * 4]
  stp     x10, x11, [sp, #16 * 5]
  stp     x12, x13, [sp, #16 * 6]
  stp     x14, x15, [sp, #16 * 7]
  stp     x16, x17, [sp, #16 * 8]
  stp     x18, x19, [sp, #16 * 9]
  stp     x20, x21, [sp, #16 * 10]
  stp     x22, x23, [sp, #16 * 11]
  stp     x24, x25, [sp, #16 * 12]
  stp     x26, x27, [sp, #16 * 13]
  stp     x28, x29, [sp, #16 * 14]
  str     x30, [sp, #16 * 15]
.endm

.macro kernel_exit
  ldp     x0, x1, [sp, #16 * 0]
  ldp     x2, x3, [sp, #16 * 1]
  ldp     x4, x5, [sp, #16 * 2]
  ldp     x6, x7, [sp, #16 * 3]
  ldp     x8, x9, [sp, #16 * 4]
  ldp     x10, x11, [sp, #16 * 5]
  ldp     x12, x13, [sp, #16 * 6]
  ldp     x14, x15, [sp, #16 * 7]
  ldp     x16, x17, [sp, #16 * 8]
  ldp     x18, x19, [sp, #16 * 9]
  ldp     x20, x21, [sp, #16 * 10]
  ldp     x22, x23, [sp, #16 * 11]
  ldp     x24, x25, [sp, #16 * 12]
  ldp     x26, x27, [sp, #16 * 13]
  ldp     x28, x29, [sp, #16 * 14]
  ldr     x30, [sp, #16 * 15]
  add     sp, sp, #16 * 16
  eret
.endm

.section ".text"

.extern handle_invalid_irq
.extern handle_irq

.global irq_vector_init
irq_vector_init:
  adr     x0, vectors                             // load VBAR_EL1 with 
  msr     vbar_el1, x0                            // vector table address
  ret

.global irq_enable
irq_enable:
  msr     daifclr, #2
  ret

.global irq_disable
irq_disable:
  msr     daifset, #2
  ret

/*
 * Exception vectors.
 */
.align 11
vectors:
  ventry  sync_invalid_el1t                       // Synchronous EL1t
  ventry  irq_invalid_el1t                        // IRQ EL1t
  ventry  fiq_invalid_el1t                        // FIQ EL1t
  ventry  error_invalid_el1t                      // Error EL1t

  ventry  el1_sync_irq                            // Synchronous EL1h
  ventry  el1_irq                                 // IRQ EL1h
  ventry  fiq_invalid_el1h                        // FIQ EL1h
  ventry  error_invalid_el1h                      // Error EL1h

  ventry  sync_invalid_el0_64                     // Synchronous 64-bit EL0
  ventry  irq_invalid_el0_64                      // IRQ 64-bit EL0
  ventry  fiq_invalid_el0_64                      // FIQ 64-bit EL0
  ventry  error_invalid_el0_64                    // Error 64-bit EL0

  ventry  sync_invalid_el0_32                     // Synchronous 32-bit EL0
  ventry  irq_invalid_el0_32                      // IRQ 32-bit EL0
  ventry  fiq_invalid_el0_32                      // FIQ 32-bit EL0
  ventry  error_invalid_el0_32                    // Error 32-bit EL0

sync_invalid_el1t:
  handle_invalid_entry  0

irq_invalid_el1t:
  handle_invalid_entry  1

fiq_invalid_el1t:
  handle_invalid_entry  2

error_invalid_el1t:
  handle_invalid_entry  3

sync_invalid_el1h:
  handle_invalid_entry  4

el1_sync_irq:
  kernel_entry
  mov x0, x8
  bl handle_sync_irq
  kernel_exit

el1_irq:
  kernel_entry
  bl handle_irq
  kernel_exit

fiq_invalid_el1h:
  handle_invalid_entry  6

error_invalid_el1h:
  handle_invalid_entry  7

sync_invalid_el0_64:
  handle_invalid_entry  8

irq_invalid_el0_64:
  handle_invalid_entry  9

fiq_invalid_el0_64:
  handle_invalid_entry  10

error_invalid_el0_64:
  handle_invalid_entry  11

sync_invalid_el0_32:
  handle_invalid_entry  12

irq_invalid_el0_32:
  handle_invalid_entry  13

fiq_invalid_el0_32:
  handle_invalid_entry  14

error_invalid_el0_32:
  handle_invalid_entry  15

/* Store the vector number in x8 register to read later */
.global irq_sync_el1
/* extern void irq_sync_el1(uint64_t vector) */
irq_sync_el1:
  mov x8, x0
  svc #0x10