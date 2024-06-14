#ifndef  __ARRAY_LIST_H
#define __ARRAY_LIST_H

#include <stddef.h>
#include <stdint.h>
#include <common/common.h>
#include <common/string.h>

#define AL_ENTRIES_PER_STRUCT 64

typedef uint64_t al_entry_t;
typedef struct array_list {
    al_entry_t * entries;
    uint8_t * base_addr;
    unsigned int num_entries;
    unsigned int max_num_entries;
    size_t list_entry_size;
} al_t;

size_t al_entries_buf_size(unsigned int max_num_entries);
int al_init(al_t *al, uint8_t * entries_buf, uint8_t * base_addr, unsigned int max_num_entries, size_t list_entry_size);
int al_entry_set(al_t * al, unsigned int index);
uint64_t al_entry_get(al_t * al, unsigned int index);
int al_entry_free(al_t * al, unsigned int index);

#endif __ARRAY_LIST_H