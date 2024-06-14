.section ".text"

.global _atomic_cmpxchg
// uint64_t *ptr, int old, int new
_atomic_cmpxchg:
1:
    mov     x6, x0
    ldaxr   x4, [x6]
    mov     x0, #1     
    eor     x1, x1, x4 
    cbnz    x1, 2f     //Not equal so dont str new val       
    stlxr   w0, x2, [x6]
2:
    ret

// These atomic_fetch_X routines follow similar format so in future
// we can turn them into macros like linux does

// uint64_t *ptr, int val
.global _atomic_fetch_add
_atomic_fetch_add:
1:
    ldaxr   x4, [x0]
    mov     x5, #0
    add     x6, x4, x1
    stlxr   w5, x6, [x0]
    cbnz    w5, 1b
    mov     x0, x6
    ret

// uint64_t *ptr, int val
.global _atomic_fetch_sub
_atomic_fetch_sub:
1:
    ldaxr   x4, [x0]
    mov     x5, #0
    sub     x6, x4, x1
    stlxr   w5, x6, [x0]
    cbnz    w5, 1b
    mov     x0, x6
    ret

// uint64_t *ptr, int val
.global _atomic_fetch_or
_atomic_fetch_or:
1:
    ldaxr   x4, [x0]
    mov     x5, #0
    orr     x6, x4, x1
    stlxr   w5, x6, [x0]
    cbnz    w5, 1b
    mov     x0, x6
    ret