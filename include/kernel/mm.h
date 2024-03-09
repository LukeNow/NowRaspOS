#ifndef __MM_H
#define __MM_H

int mm_early_init();
int mm_init(size_t mem_size, uint64_t *start_addr);
uint8_t *mm_earlypage_alloc(int num_pages);

#endif