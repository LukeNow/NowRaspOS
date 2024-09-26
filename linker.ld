ENTRY(_start)

/* Kern virtual address base, hard code the values because linker does not like
   non-constant values when calculating load addrs */
KERN_VIRTUAL_BASE = 0xffffff8000000000;

SECTIONS
{
    /* Starts at LOADER_ADDR. */
    . = 0x80000;
    __linker_start = .;

    __early_text_start = .;
    .early_text :
    {
        KEEP(*(.text.boot))
        *(.early_text)
    }
    . = ALIGN(4096);
    __early_text_end = .;
    
    __early_data_start = .;
    .early_data :
    {
        *(.early_data)
    }
    . = ALIGN(4096); /* align to page size */
    
    . += 0xffffff8000000000;
    __start = .;
    __text_start = .;
   .text ALIGN(4K) : AT(ADDR (.text) - 0xffffff8000000000)
	{	
		*(.text)
	}
    . = ALIGN(4096); /* align to page size */
    __text_end = .;

    __rodata_start = .;
   .rodata ALIGN(4K) : AT(ADDR (.rodata) - 0xffffff8000000000)
	{
		*(.rodata)
        *(.rodata.*)
	}

    . = ALIGN(4096); /* align to page size */
    __rodata_end = .;
 
    __data_start = .;
   .data ALIGN(4K) : AT(ADDR (.data) - 0xffffff8000000000)
	{
		*(.data)
	}
    . = ALIGN(4096); /* align to page size */
    __data_end = .;
 
    __bss_start = .;
    .bss ALIGN(4K) : AT(ADDR (.bss) - 0xffffff8000000000)
	{
		*(COMMON)
		*(.bss)
	}
    . = ALIGN(4096); /* align to page size */
    __bss_end = .;
    __bss_size = __bss_end - __bss_start;
    __end = .;

    /* Early data and stackpages that we want to put at the end of the phys mem space */
    . -= 0xffffff8000000000;
    __early_data_end = .;
    __stackpage_start = .;
    .  += (4096 * 4);
    __stackpage_end = .;
    __earlypage_start = .;
    .  += (4096 * 256  * 8);
    __earlypage_end = .;



    __linker_end = .;

}
