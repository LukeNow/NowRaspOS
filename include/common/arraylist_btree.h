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

/* Num_entries can be set to 0 to perform an "Inplace" list in the array pointer itself
 * This saves space for places where a lot of btree lists are used as 64bit lists
 * This also makes atomic operations on the lists themselves easy to do */
#define AL_BTREE_INPLACE 0
#define AL_BTREE_INPLACE_NUM_ENTRIES 32

typedef struct arraylist_btree {
    uint8_t * array;
    unsigned int num_entries; // Must be power of 2 or 0 for inplace operation
} al_btree_t;


int al_btree_is_full(al_btree_t * btree);
int al_btree_remove_node(al_btree_t * btree, unsigned int index);
int al_btree_add_node(al_btree_t * btree, int level);
/* Num entries can also be given as 0 to indicate "In place" operation. See above define.*/
int al_btree_init(al_btree_t * btree, void * buffer, size_t num_entries);

#endif
