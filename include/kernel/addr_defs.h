#ifndef __ADDR_DEFS_H
#define __ADDR_DEFS_H

extern void __start();
extern void __end();
extern void __data_start();
extern void __data_end();
extern void __rodata_start();
extern void __rodata_end();
extern void __text_start();
extern void __stackpage_start();
extern void __stackpage_end();
extern void __earlypage_start();
extern void __earlypage_end();
extern void __bss_start();
extern void __bss_end();
extern void __early_page_start();
extern void __early_page_end();

extern void _start();

#endif