#include <stddef.h>
#include <stdint.h>
#include <common/arraylist.h>

size_t al_entries_buf_size(unsigned int max_num_entries)
{
    return (max_num_entries / AL_ENTRIES_PER_STRUCT) * sizeof(al_entry_t);
}



int al_init(al_t *al, uint8_t * entries_buf, uint8_t * base_addr, unsigned int max_num_entries, size_t list_entry_size)
{
    memset(al, 0, sizeof(al_t));

    al->entries = (al_entry_t *)entries_buf;
    al->base_addr = base_addr;
    al->num_entries = 0;
    al->max_num_entries = max_num_entries;
    al->list_entry_size = list_entry_size;

    return 0;
}

int al_entry_set(al_t * al, unsigned int index)
{
    unsigned int struct_index = index / AL_ENTRIES_PER_STRUCT;
    unsigned int entry_index = index % AL_ENTRIES_PER_STRUCT;
    al->entries[struct_index] = al->entries[struct_index]  | ((al_entry_t)1 << entry_index);
    return 0;
}

uint64_t al_entry_get(al_t * al, unsigned int index)
{   
    unsigned int struct_index = index / AL_ENTRIES_PER_STRUCT;
    unsigned int entry_index = index % AL_ENTRIES_PER_STRUCT;
    return (uint64_t)(al->entries[struct_index] & ((uint64_t)1 << entry_index)) >> entry_index;
}

int al_entry_free(al_t * al, unsigned int index)
{
    unsigned int struct_index = index / AL_ENTRIES_PER_STRUCT;
    unsigned int entry_index = index % AL_ENTRIES_PER_STRUCT;
    al->entries[struct_index] = al->entries[struct_index]  &~ ((al_entry_t)1 << entry_index);
    return 0;
}

