ENTRY(_start)

LOADER_ADDR = 0x80000;
/* Kern virtual address base, hard code the values because linker does not like
   non-constant values when calculating load addrs */
KERN_VIRTUAL_BASE = 0xffffff8000000000;
PAGE_SIZE = 4096;
EL1_STACK_SIZE = PAGE_SIZE * 4;
EARLY_PAGE_SIZE = (PAGE_SIZE * 256 * 8);

SECTIONS
{
    /* Starts at LOADER_ADDR. */
    . = LOADER_ADDR;
    __linker_start = .;

    __early_text_start = .;
    .early_text :
    {
        KEEP(*(.text.boot))
        *(.early_text)
    }
    . = ALIGN(PAGE_SIZE);
    __early_text_end = .;
    
    __early_data_start = .;
    .early_data :
    {
        *(.early_data)
    }
    . = ALIGN(PAGE_SIZE);
    __early_data_end = .;

    /* Start linking to higher kernel adder */
    . += KERN_VIRTUAL_BASE;
    __start = .;
    __text_start = .;
   .text ALIGN(4K) : AT(ADDR (.text) - 0xffffff8000000000)
	{	
		*(.text)
	}
    . = ALIGN(PAGE_SIZE); /* align to page size */
    __text_end = .;

    __rodata_start = .;
   .rodata ALIGN(4K) : AT(ADDR (.rodata) - 0xffffff8000000000)
	{
		*(.rodata)
        *(.rodata.*)
	}

    . = ALIGN(PAGE_SIZE); /* align to page size */
    __rodata_end = .;
 
    __data_start = .;
   .data ALIGN(4K) : AT(ADDR (.data) - 0xffffff8000000000)
	{
		*(.data)
	}
    . = ALIGN(PAGE_SIZE); /* align to page size */
    __data_end = .;
 
    __bss_start = .;
    .bss ALIGN(4K) : AT(ADDR (.bss) - 0xffffff8000000000)
	{
		*(COMMON)
		*(.bss)
	}
    . = ALIGN(PAGE_SIZE); /* align to page size */
    __bss_end = .;
    __bss_size = __bss_end - __bss_start;
    __end = .;

    /* Early data and stackpages that we want to put at the end of the phys mem space */
    /* . -= 0xffffff8000000000; */
    . -= KERN_VIRTUAL_BASE;
    . = ALIGN(PAGE_SIZE);

     __stackpage_start = .;
    . += PAGE_SIZE;
    __EL2_stack_core0 = .;
    . += PAGE_SIZE;
    __EL2_stack_core1 = .;
    . += PAGE_SIZE;
    __EL2_stack_core2 = .;
    . += PAGE_SIZE;
    __EL2_stack_core3 = .;
    .  += EL1_STACK_SIZE;
    __EL1_stack_core0 = .;
    . += PAGE_SIZE;
    __EL1_stack_guard_core0 = .;
    . += EL1_STACK_SIZE;
    __EL1_stack_core1 = .;
    . += PAGE_SIZE;
    __EL1_stack_guard_core1 = .;
    . += EL1_STACK_SIZE;
    __EL1_stack_core2 = .;
    . += PAGE_SIZE;
    __EL1_stack_guard_core2 = .;
    . += EL1_STACK_SIZE;
    __EL1_stack_core3 = .;
    . += PAGE_SIZE;
    __EL1_stack_guard_core3 = .;
    . += PAGE_SIZE;
    __EL0_stack_core0 = .;
    . += PAGE_SIZE;
    __EL0_stack_core1 = .;
    . += PAGE_SIZE;
    __EL0_stack_core2 = .;
    . += PAGE_SIZE;
    __EL0_stack_core3 = .;
    /* Buffer the stack space because why not */
    . += PAGE_SIZE;
    __stackpage_end = .;
    __earlypage_start = .;
    . += EARLY_PAGE_SIZE;
    __earlypage_end = .;
    __earlydata_start = .;
    /* We will have an early data heap here that we will keep track off in our memory allocator */
    __linker_end = .;
}
