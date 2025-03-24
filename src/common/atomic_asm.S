.section ".text"

// semaphore_take(uint64_t * count)
.global atomic_semaphore_take
.type atomic_semaphore_take, %function
atomic_semaphore_take:
    mov x3, x0
1:
    ldaxr x1, [x3]
    cbz x1, 1b   // if zero we spin
    mov x2, #1
    sub x1, x1, x2
    stlxr w0, x1, [x3]
    cbnz w0, 1b
    ret

// semaphore_give(uint64_t * count)
.global atomic_semaphore_give
.type atomic_semaphore_give, %function
atomic_semaphore_give:
    mov x3, x0
1:
    ldaxr x1, [x3]
    mov x2, #1
    add x1, x1, x2
    stlxr w0, x1, [x3]
    cbnz w0, 1b
    ret

// uint64_t *ptr, uint64_t old, uint64_t new
.global atomic_cmpxchg_64
.type atomic_cmpxchg_64, %function
atomic_cmpxchg_64:
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
.global atomic_fetch_add_64
.type atomic_fetch_add_64, %function
atomic_fetch_add_64:
1:
    ldaxr   x4, [x0]
    mov     x5, #0
    add     x6, x4, x1
    stlxr   w5, x6, [x0]
    cbnz    w5, 1b
    mov     x0, x6
    ret

// uint64_t *ptr, int val
.global atomic_fetch_sub_64
.type atomic_fetch_sub_64, %function
atomic_fetch_sub_64:
1:
    ldaxr   x4, [x0]
    mov     x5, #0
    sub     x6, x4, x1
    stlxr   w5, x6, [x0]
    cbnz    w5, 1b
    mov     x0, x6
    ret

// uint64_t *ptr, int val
.global atomic_fetch_or_64
.type atomic_fetch_or_64, %function
atomic_fetch_or_64:
1:
    ldaxr   x4, [x0]
    mov     x5, #0
    orr     x6, x4, x1
    stlxr   w5, x6, [x0]
    cbnz    w5, 1b
    mov     x0, x6
    ret

// uint32_t * ptr
.global atomic_ld_32
.type atomic_ld_32, %function
atomic_ld_32:
    ldaxr w6, [x0]
    mov w0, w6
    ret

// uint64_t *ptr
.global atomic_ld_64
.type atomic_ld_64, %function
atomic_ld_64:
    ldaxr x6, [x0]
    mov x0, x6
    ret

// uint32_t * ptr, uint32_t new_val
.global atomic_str_32
.type atomic_str_32, %function
atomic_str_32:
    stxr w3, w1, [x0]
    mov w0, w3
    ret

// uint64_t * ptr, uint64_t new_val
.global atomic_str_64
.type atomic_str_64, %function
atomic_str_64:
    stxr w3, x1, [x0]
    mov w0, w3
    ret
