#ifndef __ARRAYLIST_BTREE_H
#define __ARRAYLIST_BTREE_H

#include <stddef.h>
#include <common/common.h>
#include <common/math.h>

#define LEFT_CHILD(INDEX) (2*(INDEX)+1)
#define RIGHT_CHILD(INDEX) (2*(INDEX)+2)
#define PARENT(INDEX) (((INDEX)-1)/2)

#define AL_BTREE_ENTRY_SIZE 2 //bits
#define ENTRY_SIZE_BYTES 1
#define ENTRY_ARRAY_INDEX(INDEX) ((AL_BTREE_ENTRY_SIZE * (INDEX)) / BITS_PER_BYTE)

#define AL_ENTRY_UNUSED 0
#define AL_ENTRY_SPLIT 1
#define AL_ENTRY_FULL 3
typedef enum {unused = AL_ENTRY_UNUSED, split = AL_ENTRY_SPLIT, full = AL_ENTRY_FULL } al_btree_entry_t;

#define AL_NULL_INDEX -1

/* Number of entries is the number of ALL nodes in the list, by extension the number of leaf nodes is half this. */
typedef struct arraylist_btree {
    uint8_t * array;
    size_t array_size; //In Bytes
    size_t num_entries; // Must be power of 2
} al_btree_t;


int al_btree_is_full(al_btree_t * btree);
int al_btree_remove_node(al_btree_t * btree, unsigned int index);
int al_btree_add_node(al_btree_t * btree, int level);
int al_btree_init(al_btree_t * btree, void * buffer, size_t num_entries);

#endif
