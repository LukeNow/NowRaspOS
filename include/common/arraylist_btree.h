#ifndef __ARRAYLIST_BTREE_H
#define __ARRAYLIST_BTREE_H

#include <stddef.h>
#include <common/common.h>
#include <common/math.h>

#define AL_BTREE_MAX_LEVEL 5 //Log2(64) - 1, with 0th level at root
#define AL_BTREE_NUM_ENTRIES 63
#define AL_BTREE_MAX_LEVEL_ENTRIES 32
#define AL_BTREE_MAX_LEVEL_START_INDEX 31
#define AL_BTREE_NULL_INDEX -1

typedef uint64_t al_btree_entry_t;

typedef struct arraylist_btree {
    uint64_t list;
} al_btree_t;


unsigned int al_btree_level_is_full(al_btree_t * btree, unsigned int level);
int al_btree_remove_node(al_btree_t * btree, unsigned int index);
int al_btree_add_node(al_btree_t * btree, int level);
int al_btree_init(al_btree_t * btree);

#endif
