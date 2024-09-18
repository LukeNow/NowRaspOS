#ifndef __MMU_H
#define __MMU_H

#include <stdint.h>
#include <stddef.h>
#include <common/bits.h>

#define MMU_VA_SIZE_BITS 39
#define MMU_UPPER_ADDRESS (BITS(64 - MMU_VA_SIZE_BITS) << MMU_VA_SIZE_BITS)

#define MT_DEVICE_NGNRNE	0
#define MT_DEVICE_NGNRE		1
#define MT_DEVICE_GRE		2
#define MT_NORMAL_NC		3
#define MT_NORMAL		    4

#define PAGE_OFF 12
#define PAGE_MASK (PAGE_SIZE - 1)

#define PT_ADDR (BITS(36) << 12)
#define PT_SET_ADDR(ADDR) (((ADDR) & PAGE_MASK) << 12)

#define PT_ENTRY    (3 << 0)
#define PT_PAGE     (3 << 0)     
#define PT_BLOCK    (1 << 0)
#define PT_MEM_ATTR(ATTR) ((ATTR) << 2)
#define PT_NO_WRITE_EL0 (2 << 6)
#define PT_NO_READ_EL0 (1 << 6)
#define PT_OUTER_SHAREABLE (2 << 8)
#define PT_INNER_SHAREABLE (3 << 8)
#define PT_ACCESSED (1 << 10)
#define PT_NO_EXECUTE (1 << 54)
#define PT_NO_EXECUTE_LOWER_EL (1 << 59)
#define PT_NO_TRANSLATE_LOWER_EL (1 << 60)
#define PT_SECURE (1 << 63)

#define MAIR1_VAL ((0x00ul << (MT_DEVICE_NGNRNE * 8)) |\
                 (0x04ul << (MT_DEVICE_NGNRE * 8)) |\
				 (0x0cul << (MT_DEVICE_GRE * 8)) |\
                 (0x44ul << (MT_NORMAL_NC * 8)) |\
				 (0xfful << (MT_NORMAL * 8)) )

#define SCTLREL1_VAL ( (0xC00800) |		/* set mandatory reserved bits */\
					  (1 << 12)  |      /* I, Instruction cache enable. This is an enable bit for instruction caches at EL0 and EL1 */\
					  (1 << 4)   |		/* SA0, tack Alignment Check Enable for EL0 */\
					  (1 << 3)   |		/* SA, Stack Alignment Check Enable */\
					  (1 << 2)   |		/* C, Data cache enable. This is an enable bit for data caches at EL0 and EL1 */\
					  (1 << 1)   |		/* A, Alignment check enable bit */\
					  (1 << 0) )		/* set M, enable MMU */

#define TCREL1_VAL  ( (0b00LL << 37) |   /* TBI=0, no tagging */\
					 (0b001LL << 32) |  /* IPS= 32 bit ... 000 = 32bit, 001 = 36bit, 010 = 40bit */\
					 (0b10LL << 30)  |  /* TG1=4k ... options are 10=4KB, 01=16KB, 11=64KB ... take care differs from TG0 */\
					 (0b11LL << 28)  |  /* SH1=3 inner ... options 00 = Non-shareable, 01 = INVALID, 10 = Outer Shareable, 11 = Inner Shareable */\
					 (0b01LL << 26)  |  /* ORGN1=1 write back .. options 00 = Non-cacheable, 01 = Write back cacheable, 10 = Write thru cacheable, 11 = Write Back Non-cacheable */\
					 (0b01LL << 24)  |  /* IRGN1=1 write back .. options 00 = Non-cacheable, 01 = Write back cacheable, 10 = Write thru cacheable, 11 = Write Back Non-cacheable */\
					 (0b0LL  << 23)  |  /* EPD1 ... Translation table walk disable for translations using TTBR1_EL1  0 = walk, 1 = generate fault */\
					 (25LL   << 16)  |  /* T1SZ=25 (512G) ... The region size is 2 POWER (64-T1SZ) bytes */\
					 (0b00LL << 14)  |  /* TG0=4k  ... options are 00=4KB, 01=64KB, 10=16KB,  ... take care differs from TG1 */\
					 (0b11LL << 12)  |  /* SH0=3 inner ... .. options 00 = Non-shareable, 01 = INVALID, 10 = Outer Shareable, 11 = Inner Shareable */\
					 (0b01LL << 10)  |  /* ORGN0=1 write back .. options 00 = Non-cacheable, 01 = Write back cacheable, 10 = Write thru cacheable, 11 = Write Back Non-cacheable */\
					 (0b01LL << 8)   |  /* IRGN0=1 write back .. options 00 = Non-cacheable, 01 = Write back cacheable, 10 = Write thru cacheable, 11 = Write Back Non-cacheable */\
					 (0b0LL  << 7)   |  /* EPD0  ... Translation table walk disable for translations using TTBR0_EL1  0 = walk, 1 = generate fault */\
					 (25LL   << 0) ) 	/* T0SZ=25 (512G)  ... The region size is 2 POWER (64-T0SZ) bytes */


typedef union {
	struct {
		uint64_t EntryType : 2;				// @0-1		1 for a block table, 3 for a page table
		    
			/* These are only valid on BLOCK DESCRIPTOR */
			uint64_t MemAttr : 4;			// @2-5
			enum {
				STAGE2_S2AP_NOREAD_EL0 = 1,	//			No read access for EL0
				STAGE2_S2AP_NO_WRITE = 2,	//			No write access
			} S2AP : 2;						// @6-7
			enum {
				STAGE2_SH_OUTER_SHAREABLE = 2,	//			Outter shareable
				STAGE2_SH_INNER_SHAREABLE = 3,	//			Inner shareable
			} SH : 2;						// @8-9
			uint64_t AF : 1;				// @10		Accessable flag

		uint64_t _reserved11 : 1;			// @11		Set to 0
		uint64_t Address : 36;				// @12-47	36 Bits of address
		uint64_t _reserved48_51 : 4;		// @48-51	Set to 0
		uint64_t Contiguous : 1;			// @52		Contiguous
		uint64_t _reserved53 : 1;			// @53		Set to 0
		uint64_t XN : 1;					// @54		No execute if bit set
		uint64_t _reserved55_58 : 4;		// @55-58	Set to 0
		
		uint64_t PXNTable : 1;				// @59      Never allow execution from a lower EL level 
		uint64_t XNTable : 1;				// @60		Never allow translation from a lower EL level
		enum {
			APTABLE_NOEFFECT = 0,			// No effect
			APTABLE_NO_EL0 = 1,				// Access at EL0 not permitted, regardless of permissions in subsequent levels of lookup
			APTABLE_NO_WRITE = 2,			// Write access not permitted, at any Exception level, regardless of permissions in subsequent levels of lookup
			APTABLE_NO_WRITE_EL0_READ = 3	// Write access not permitted,at any Exception level, Read access not permitted at EL0.
		} APTable : 2;						// @61-62	AP Table control .. see enumerate options
		uint64_t NSTable : 1;				// @63		Secure state, for accesses from Non-secure state this bit is RES0 and is ignored
	};
	uint64_t Raw64;							// @0-63	Raw access to all 64 bits via this union
} VMSAv8_64_DESCRIPTOR;

int mmu_init(uint32_t phy_mem_size, uint32_t vc_mem_start, uint32_t vc_mem_size);

#endif