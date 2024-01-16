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
    cbz     x1, 2f
    // cpu id > 0, stop
1:  wfe
    b       1b
2:  // cpu id == 0

    // set top of stack just before our code (stack grows to a lower address per AAPCS64)
    ldr     x1, =_start
    mov     sp, x1

    // clear bss
    ldr     x1, =__bss_start
    ldr     w2, =__bss_size
3:  cbz     w2, 4f
    str     xzr, [x1], #8
    sub     w2, w2, #8
    cbnz    w2, 3b

    // jump to C code, should not return
    
    
4:  mov     x0, x12
    mov     x1, #0x0
    mov     x2, #0x0
    mov     x3, #0x0
    bl      kernel_main
    // for failsafe, halt this core too
    b       1b
