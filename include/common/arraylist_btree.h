#ifndef __ARRAYLIST_BTREE_H
#define __ARRAYLIST_BTREE_H

#include <stddef.h>
#include <common/common.h>
#include <common/math.h>

#define AL_BTREE_MAX_LEVEL 5 //Log2(64) - 1, with 0th level at root
#define AL_BTREE_LEVEL_NUM (AL_BTREE_MAX_LEVEL + 1)
#define AL_BTREE_NUM_ENTRIES 63
#define AL_BTREE_MAX_LEVEL_ENTRIES 32
#define AL_BTREE_MAX_LEVEL_START_INDEX 31
#define AL_BTREE_NULL_INDEX -1
#define AL_BTREE_FULL 0x7FFFFFFFFFFFFFFF
#define AL_BTREE_EMPTY 0
#define AL_BTREE_STATUS_EMPTY 0
#define AL_BTREE_STATUS_PARTIAL 1
#define AL_BTREE_STATUS_FULL 2

#define AL_BTREE_LEVEL_ENTRIES_NUM(X) (1 << X)

typedef uint64_t al_btree_entry_t; //Define as uint64_t so that we dont have typecasting problems even though the entries themselves are 1 bit
typedef uint64_t al_btree_t;
typedef struct al_btree_scan {
    int level_count[AL_BTREE_LEVEL_NUM];
} al_btree_scan_t;

unsigned int al_btree_level_is_full(al_btree_t * btree, unsigned int level);
unsigned int al_btree_level_is_empty(al_btree_t * btree, unsigned int level);
unsigned int al_btree_is_empty(al_btree_t * btree);
unsigned int al_btree_is_full(al_btree_t * btree);
unsigned int al_btree_index_to_level(unsigned int index);
unsigned int al_btree_level_to_index(unsigned int level);
unsigned int al_btree_level_to_entries_num(unsigned int level);
unsigned int al_btree_index_to_offset(unsigned int index);
int al_btree_scan_count(al_btree_t * btree, al_btree_scan_t * scan);
int al_btree_scan_merge(al_btree_scan_t * count_scan, al_btree_scan_t * alloc_scan);
int al_btree_remove_node(al_btree_t * btree, al_btree_scan_t * scan, unsigned int index);
int al_btree_atomic_remove_node(al_btree_t * btree, al_btree_scan_t * scan, unsigned int index);
int al_btree_add_node(al_btree_t * btree, al_btree_scan_t * scan, int level);
int al_btree_atomic_add_node(al_btree_t * btree, al_btree_scan_t * scan, int level);
int al_btree_atomic_cmpxchg(al_btree_t *btree, al_btree_t expected, al_btree_t target);
int al_btree_init(al_btree_t * btree);

#endif
