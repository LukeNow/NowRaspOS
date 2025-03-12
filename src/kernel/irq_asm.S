.include "src/kernel/cpu_asm_defs.s"

.extern uart_hex

.macro handle_invalid_entry type
  TASK_SAVE_IRQ_CONTEXT
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

.section ".text"

.extern handle_invalid_irq
.extern handle_irq
.extern handle_sync_irq

.global irq_vector_init
irq_vector_init:
  adr x0, vectors                             // load VBAR_EL1 with 
  msr vbar_el1, x0                            // vector table address
  ret

.global irq_enable
irq_enable:
  msr daifclr, #2
  ret

.global irq_disable
irq_disable:
  msr daifset, #2
  ret

/* irq_save(uint64_t * flags) */
.global irq_save
  mrs x3, daif
  msr daifset, #2
  str x3, [x0]
  ret

/* irq_save_disable(uint64_t * flags) */
.global irq_save_disable
irq_save_disable:
  mrs x3, daif
  msr daifset, #2
  str x3, [x0]
  ret

/* irq_restore(uint64_t flags) */
.global irq_restore
irq_restore:
  msr daif, x0
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
  TASK_SAVE_IRQ_CONTEXT
  
  /* Align the stack ptr to 16 bytes as irqs require alignment */
  mov x1, sp
  and x1, x1, 0xF
  sub sp, sp, x1
  stp x1, xzr, [SP, #-16]

  mov x0, x8
  bl handle_sync_irq

  /* Restore the stack ptr */
  ldp x1, xzr, [sp], #16
  add sp, sp, x1

  TASK_RESTORE_CONTEXT
  /* No ret */

el1_irq:
  TASK_SAVE_IRQ_CONTEXT

  /* Align the stack ptr to 16 bytes as irqs require alignment */
  mov x1, sp
  and x1, x1, 0xF
  sub sp, sp, x1
  stp x1, xzr, [SP, #-16]

  bl handle_irq

  /* Restore the stack ptr */
  ldp x1, xzr, [sp], #16
  add sp, sp, x1

  TASK_RESTORE_CONTEXT
  /* No ret */

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