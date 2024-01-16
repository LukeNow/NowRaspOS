.section ".text"

.global _atomic_cmpxchg
// uint64_t *ptr, int old, int new, int *currval
_atomic_cmpxchg:
1:  
    ldaxr   x4, [x0]
    str     x4, [x3]       
    mov     x5, #0
    eor     x1, x1, x4 
    cbnz    x1, 2f      //Not equal so dont str new val       
    stlxr   w5, x2, [x0]
    mov     w0, w5
    cbnz    w0, 1b
    ret
2:  
    mov x0, #0
    ret

