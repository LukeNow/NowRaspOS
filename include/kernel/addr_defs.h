#ifndef __ADDR_DEFS_H
#define __ADDR_DEFS_H

#define EARLY_TEXT __attribute__ ((section (".early_text")))
#define EARLY_DATA(VAR) VAR __attribute__ ((section (".early_data")))

EARLY_DATA(extern void __start());
EARLY_DATA(extern void __end());
EARLY_DATA(extern void __data_start());
EARLY_DATA(extern void __data_end());
EARLY_DATA(extern void __rodata_start());
EARLY_DATA(extern void __rodata_end());
EARLY_DATA(extern void __text_start());
EARLY_DATA(extern void __stackpage_start());
EARLY_DATA(extern void __stackpage_end());
EARLY_DATA(extern void __earlypage_start());
EARLY_DATA(extern void __earlypage_end());
EARLY_DATA(extern void __bss_start());
EARLY_DATA(extern void __bss_end());
EARLY_DATA(extern void __early_page_start());
EARLY_DATA(extern void __early_page_end());
EARLY_DATA(extern void _start());

#endif