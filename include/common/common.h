#ifndef __COMMON_H
#define __COMMON_H

#define CYCLE_WAIT(x) do { int i = (x); while (i--) { asm volatile ("nop"); } } while (0)
#define CYCLE_INFINITE do {  while(1){ asm volatile ("nop"); } } while (0)

typedef uint32_t flags_t;

#define KB(X) (X * 1024)
#define MB(X) (X * KB(1) * 1024)
#define GB(X) (X * MB(1) * 1024)

#define IS_ALIGNED(X, Y) ((X) % (Y) ? 0 : 1)
#define ROUND_DOWN(x, size) ((x) - ((x) % (size)))

#define ROUND_UP(x, size) (IS_ALIGNED((x), (size)) ? \
		(x) : ((x) + ((size) - ((x) % (size)))))
#define ALIGN_DOWN(x, size) (x >= size ? (ROUND_DOWN((x), (size))) : 0)
#define ALIGN_UP(x, size) ROUND_UP((x), (size))

/* Get the struct pointer given a pointer, what the variable is called in the struct definition, and the struct type. */
#define STRUCT_P(member_p, struct_type, member) (struct_type *)((uint8_t*)member_p - offsetof(struct_type, member))

#define PTR_IN_RANGE(PTR, RANGE_PTR_START, RANGE_SIZE) (((uint64_t)(PTR) >= (uint64_t)(RANGE_PTR_START)) && ((uint64_t)(PTR) < ((uint64_t)(RANGE_PTR_START)) + (RANGE_SIZE)))
#define VAL_IN_RANGE(VAL, RANGE_START, RANGE_SIZE) (((VAL) >= (RANGE_START)) && ((VAL) < (RANGE_START) + (RANGE_SIZE)))
#define VAL_IN_RANGE_INCLUSIVE(VAL, RANGE_START, RANGE_SIZE) (((VAL) >= (RANGE_START)) && ((VAL) <= (RANGE_START) + (RANGE_SIZE)))
#define RANGES_OVERLAP_INCLUSIVE(RANGE1_START, RANGE1_SIZE, RANGE2_START, RANGE2_SIZE) \
		(((RANGE1_START) >= ((RANGE2_START) + (RANGE2_SIZE))) && (((RANGE1_START) + (RANGE1_SIZE)) >= (RANGE2_START)))
#define RANGES_OVERLAP_EXCLUSIVE(RANGE1_START, RANGE1_SIZE, RANGE2_START, RANGE2_SIZE) \
		(((RANGE1_START) > ((RANGE2_START) + (RANGE2_SIZE))) && (((RANGE1_START) + (RANGE1_SIZE)) > (RANGE2_START)))

#define GET32(GET_ADDR) (*((uint32_t *)(GET_ADDR)))
#define GET64(GET_ADDR) (*((uint64_t *)(GET_ADDR)))
#define PUT32(TO_ADDR, VAL) (*((uint32_t *)(TO_ADDR)) = (VAL))
#define PUT64(TO_ADDR, VAL) (*((uint64 *)(TO_ADDR)) = (VAL))

#endif