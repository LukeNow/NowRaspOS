.section ".text"

.include "src/kernel/cpu_asm_defs.s"

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


.extern sched_enter
.extern sched_exit
.extern sched_task_select
.extern sched_task_switch
/* void task_start()*/
.global task_start
/* void task_switch_async() */
.global task_switch_async
/* void task_Switch_sync() */
.global task_switch_sync

task_switch_sync:
	TASK_SAVE_SYNC_CONTEXT
task_switch_async:
	bl sched_task_select
	ldr x1, [x0] /* (uint64_t/el1_stack_ptr) x2 = *(task_t)curr_task */
    mov sp, x1
	bl sched_task_switch

/* FALL THROUGH INTENTIONAL */
/* restore the context of the ready task that is now in cpu->curr_task */
/* Current CPU task must be configured to stack ptr we are jumping to. */
task_start:
    TASK_RESTORE_CONTEXT

	

