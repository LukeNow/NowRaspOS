.section ".text"

.extern cpu_get_currcpu_info

.macro TASK_SAVE_IRQ_CONTEXT
    STP 	X0, X1, [SP, #-0x10]!
	STP 	X2, X3, [SP, #-0x10]!
	STP 	X4, X5, [SP, #-0x10]!
	STP 	X6, X7, [SP, #-0x10]!
	STP 	X8, X9, [SP, #-0x10]!
	STP 	X10, X11, [SP, #-0x10]!
	STP 	X12, X13, [SP, #-0x10]!
	STP 	X14, X15, [SP, #-0x10]!
	STP 	X16, X17, [SP, #-0x10]!
	STP 	X18, X19, [SP, #-0x10]!
	STP 	X20, X21, [SP, #-0x10]!
	STP 	X22, X23, [SP, #-0x10]!
	STP 	X24, X25, [SP, #-0x10]!
	STP 	X26, X27, [SP, #-0x10]!
	STP 	X28, X29, [SP, #-0x10]!
	STP 	X30, XZR, [SP, #-0x10]!

    MRS		X3, SPSR_EL1
	MRS		X2, ELR_EL1
	STP 	X2, X3, [SP, #-0x10]!

    bl cpu_get_currcpu_info

    ldr x1, [x0]
    /* Store the current stack pointer into the current task stack top */
    mov x2, sp
    str x2, [x1]
.endm


.macro TASK_SAVE_SYNC_CONTEXT
    STP 	X0, X1, [SP, #-0x10]!
	STP 	X2, X3, [SP, #-0x10]!
	STP 	X4, X5, [SP, #-0x10]!
	STP 	X6, X7, [SP, #-0x10]!
	STP 	X8, X9, [SP, #-0x10]!
	STP 	X10, X11, [SP, #-0x10]!
	STP 	X12, X13, [SP, #-0x10]!
	STP 	X14, X15, [SP, #-0x10]!
	STP 	X16, X17, [SP, #-0x10]!
	STP 	X18, X19, [SP, #-0x10]!
	STP 	X20, X21, [SP, #-0x10]!
	STP 	X22, X23, [SP, #-0x10]!
	STP 	X24, X25, [SP, #-0x10]!
	STP 	X26, X27, [SP, #-0x10]!
	STP 	X28, X29, [SP, #-0x10]!
	STP 	X30, XZR, [SP, #-0x10]!

    MRS		X3, SPSR_EL1
	/* Store the link register as we are making a syncronous call back to a function */
	mov 	X2, x30
	STP 	X2, X3, [SP, #-0x10]!

    bl cpu_get_currcpu_info

	ldr x1, [x0]
    /* Store the current stack pointer into the current task stack top */
    mov x2, sp
    str x2, [x1]
.endm

.macro TASK_RESTORE_CONTEXT
    bl cpu_get_currcpu_info
    ldr x1, [x0]
    ldr x2, [x1]
    mov sp, x2

    ldp x2, x3, [sp], #0x10

    /* Restore spsr, elr */
    msr spsr_el1, x3
    msr elr_el1, x2

    LDP 	X30, XZR, [SP], #0x10
	LDP 	X28, X29, [SP], #0x10
	LDP 	X26, X27, [SP], #0x10
	LDP 	X24, X25, [SP], #0x10
	LDP 	X22, X23, [SP], #0x10
	LDP 	X20, X21, [SP], #0x10
	LDP 	X18, X19, [SP], #0x10
	LDP 	X16, X17, [SP], #0x10
	LDP 	X14, X15, [SP], #0x10
	LDP 	X12, X13, [SP], #0x10
	LDP 	X10, X11, [SP], #0x10
	LDP 	X8, X9, [SP], #0x10
	LDP 	X6, X7, [SP], #0x10
	LDP 	X4, X5, [SP], #0x10
	LDP 	X2, X3, [SP], #0x10
	LDP 	X0, X1, [SP], #0x10

    /* return to the addresses that was saved in the elr */
    eret
.endm

.global task_init_stack
/* uint64_t * task_init_stack(uint64_t * stack_addr, uint64_t * task_start, void * params) */
task_init_stack:
    mov x3, #0x0101010101010101
	str x3, [x0, #-8]!           //  X1
	str x2, [x0, #-8]!           //  X0 = pvParameters
	mov x3, #0x0303030303030303
	str x3, [x0, #-8]!           //  X3
	mov x3, #0x0202020202020202
	str x3, [x0, #-8]!           //  X2
	ldr x3, #=0x0505050505050505
	str x3, [x0, #-8]!           //  X5
	mov x3, #0x0404040404040404
	str x3, [x0, #-8]!           //  X4
	mov x3, #0x0707070707070707
	str x3, [x0, #-8]!           //  X7
	mov x3, #0x0606060606060606
	str x3, [x0, #-8]!           //  X6
	ldr x3, #=0x0909090909090909
	str x3, [x0, #-8]!           //  X9
	mov x3, #0x0808080808080808
	str x3, [x0, #-8]!           //  X8
	mov x3, #0x1111111111111111
	str x3, [x0, #-8]!           //  X11
	mov x3, #0x1010101010101010
	str x3, [x0, #-8]!           //  X10
	ldr x3, #=0x1313131313131313
	str x3, [x0, #-8]!           //  X13
	ldr x3, #=0x1212121212121212
	str x3, [x0, #-8]!           //  X12
	ldr x3, #=0x1515151515151515
	str x3, [x0, #-8]!           //  X15
	ldr x3, #=0x1414141414141414
	str x3, [x0, #-8]!           //  X14
	ldr x3, #=0x1717171717171717
	str x3, [x0, #-8]!           //  X17
	ldr x3, #=0x1616161616161616
	str x3, [x0, #-8]!           //  X16
	ldr x3, #=0x1919191919191919
	str x3, [x0, #-8]!           //  X19
	mov x3, #0x1818181818181818
	str x3, [x0, #-8]!           //  X18
	ldr x3, #=0x2121212121212121
	str x3, [x0, #-8]!           //  X21
	mov x3, #0x2020202020202020
	str x3, [x0, #-8]!           //  X20
	ldr x3, #=0x2323232323232323
	str x3, [x0, #-8]!           //  X23
	mov x3, #0x2222222222222222
	str x3, [x0, #-8]!           //  X22
	ldr x3, #=0x2525252525252525
	str x3, [x0, #-8]!           //  X25
	ldr x3, #=0x2424242424242424
	str x3, [x0, #-8]!           //  X24
	ldr x3, #=0x2727272727272727
	str x3, [x0, #-8]!           //  X27
	ldr x3, #=0x2626262626262626
	str x3, [x0, #-8]!           //  X26
	ldr x3, #=0x2929292929292929
	str x3, [x0, #-8]!           //  X29
	ldr x3, #=0x2828282828282828
	str x3, [x0, #-8]!           //  X28
	mov x3, #0x0000000000000000
	str x3, [x0, #-8]!           //  XZR
	mov x3, #0x0000000000000000
	str x3, [x0, #-8]!           //  X30

    //mrs		x3, spsr_el1
	mov x3, #0x345              // Initial PSTATE
	str x3, [x0, #-8]!           
	str x1, [x0, #-8]!           // code addr
	ret

.extern sched_pull_ready_task
/* void task_start()*/
.global task_start
/* void task_switch_async() */
.global task_switch_async
/* void task_Switch_sync() */
.global task_switch_sync

task_switch_sync:
	TASK_SAVE_SYNC_CONTEXT
task_switch_async:
	bl sched_pull_ready_task
/* FALL THROUGH INTENTIONAL */
/* restore the context of the ready task that is now in cpu->curr_task */
/* Current CPU task must be configured to stack ptr we are jumping to. */
task_start:
    TASK_RESTORE_CONTEXT

	

