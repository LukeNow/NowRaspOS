#include <stddef.h>
#include <stdint.h>
#include <common/arraylist_btree.h>
#include <common/common.h>
#include <common/assert.h>
#include <common/math.h>
#include <common/string.h>
#include <common/bits.h>

#define LEFT_CHILD(INDEX) (2*(INDEX)+1)
#define RIGHT_CHILD(INDEX) (2*(INDEX)+2)
#define DEFAULT_CHILD(INDEX) (LEFT_CHILD((INDEX))) //The "default" child as its the earlier index of the children
#define PARENT(INDEX) (((INDEX)-1)/2)
#define SIBLING(INDEX) (((INDEX-1)^1)+1)

#define TREE_LVL_0_MASK 0b1
#define TREE_LVL_1_MASK 0b110
#define TREE_LVL_2_MASK 0b1111000
#define TREE_LVL_3_MASK 0b111111110000000
#define TREE_LVL_4_MASK 0b1111111111111111000000000000000
#define TREE_LVL_5_MASK 0b0111111111111111111111111111111110000000000000000000000000000000


static const uint64_t level_to_mask[] = {
    TREE_LVL_0_MASK,
    TREE_LVL_1_MASK,
    TREE_LVL_2_MASK, 
    TREE_LVL_3_MASK,
    TREE_LVL_4_MASK,
    TREE_LVL_5_MASK
};

static const unsigned int level_to_index[] = {
    0, 1, 3, 7, 15, 31, 63
};

static inline al_btree_entry_t _al_btree_get_array_entry64(uint64_t * list, unsigned int index, size_t size)
{   
    return (uint64_t)(*list & ((uint64_t)BITS(size) << index)) >> index;
}

static inline void _al_btree_set_array_entry64(uint64_t * list, unsigned int index, al_btree_entry_t entry, size_t size)
{   
    // Clear the entry, then set the entry with the data
    *list = (*list &~ ((uint64_t)BITS(size) << index)) | ((uint64_t)entry << index);
}

static inline al_btree_entry_t _al_btree_get_array_entry32(uint32_t * list, unsigned int index, size_t size)
{
    return (uint32_t)(*list & ((uint32_t)BITS(size) << index)) >> index;
}

static inline void _al_btree_set_array_entry32(uint32_t * list, unsigned int index, al_btree_entry_t entry, size_t size)
{ 
    // Clear the entry, then set the entry with the data
    *list = (*list &~ ((uint32_t)BITS(size)<< index)) | ((uint32_t)entry << index);
}

unsigned int al_btree_index_to_level(unsigned int index)
{   
    unsigned int i;
    for (i = 0; i < AL_BTREE_MAX_LEVEL + 1; i++) {
        if (index < level_to_index[i + 1])
            break;
    }

    return i;
}

unsigned int al_btree_level_to_index(unsigned int level) {
    ASSERT(level <= AL_BTREE_MAX_LEVEL + 1);
    
    return level_to_index[level];
}

unsigned int al_btree_level_to_entries_num(unsigned int level) {
    ASSERT(level <= AL_BTREE_MAX_LEVEL);

    return 1 << level;
}

// Offset of the index relative to the level
unsigned int al_btree_index_to_offset(unsigned int index)
{
    unsigned int level = al_btree_index_to_level(index);
    return index - level_to_index[level];
}

int al_btree_init(al_btree_t * btree)
{
    if (!btree) {
        ASSERT(0);
        return 1;
    }

    memset(btree, 0, sizeof(al_btree_t));

    return 0;
}

unsigned int al_btree_is_full(al_btree_t * btree)
{
    return *btree == AL_BTREE_FULL;
}

unsigned int al_btree_is_empty(al_btree_t * btree)
{
    return *btree == AL_BTREE_EMPTY;
}

unsigned int al_btree_level_is_full(al_btree_t * btree, unsigned int level)
{
    uint64_t level_mask = level_to_mask[level];
    
    return (*btree & level_mask) == level_mask;
}

unsigned int al_btree_level_is_empty(al_btree_t * btree, unsigned int level)
{
    uint64_t level_mask = level_to_mask[level];

    return (*btree & level_mask) == AL_BTREE_EMPTY;
}

// Merge the count and the alloc scans to get an updated count of the tree after an alloc
int al_btree_scan_merge(al_btree_scan_t * count_scan, al_btree_scan_t * alloc_scan)
{
    if (!count_scan || !alloc_scan) {
        ASSERT(0);
        DEBUG("AL btree combine scan inputs are null");
        return 1;
    }

    for (int i = 0; i < AL_BTREE_LEVEL_NUM; i++) {
        count_scan->level_count[i] += alloc_scan->level_count[i];
    }

    return 0;
}

// Scans the total count for each level of the tree
int al_btree_scan_count(al_btree_t * btree, al_btree_scan_t * scan)
{
    if (!btree || !scan) {
        ASSERT(0);
        DEBUG("AL btree scan input is null");
        return 1;
    }

    for (int i = 0; i < AL_BTREE_LEVEL_NUM; i++) {
        uint64_t level_masked = (*btree & level_to_mask[i]) >> level_to_index[i];
        scan->level_count[i] = bits_count_32(level_masked);
    }

    return 0;
}

void _al_btree_add_array_entry(al_btree_t * btree,  al_btree_scan_t * scan, unsigned int index, unsigned int level, al_btree_entry_t entry)
{
    unsigned int child_index, parent_index, curr_index, sibling_index, curr_level, curr_size, curr_entry;
    uint64_t level_mask;
    
    ASSERT(btree && index < AL_BTREE_NUM_ENTRIES);    

    ASSERT_PANIC(_al_btree_get_array_entry64(btree, index, 1) != entry, "Entry is being set at the same value, double free? double assign?");

    _al_btree_set_array_entry64(btree, index, entry, 1);
    
    // If we are passed a scan entry, we should fill it out with the changes we make to each level
    if (scan && entry) {
        scan->level_count[level] = 1;
    } else if (scan) {
        scan->level_count[level] = -1;
    }

    curr_index = index;
    curr_size = 2;

    // Go down the tree and set all children of the node so that they cannot be reserved
    for (int i = level + 1; i < AL_BTREE_LEVEL_NUM; i++) {
        curr_entry = (entry ? BITS(curr_size) : 0);
        child_index = DEFAULT_CHILD(curr_index);
        _al_btree_set_array_entry64(btree, child_index, curr_entry, curr_size);
        
        if (scan && curr_entry) {
            scan->level_count[i] = curr_size;
        } else if (scan) {
            scan->level_count[i] = -curr_size;
        }

        curr_index = child_index;
        curr_size = curr_size << 1;
    }
    
    curr_index = index;
    curr_size = 1;

    //Go up the tree and set the parents that they are also unavailable. However we must check sibling nodes if the parent node is actually free/taken
    for (int i = level; i > 0; i--) {
        curr_entry = (entry ? BITS(curr_size) : 0);
        parent_index = PARENT(curr_index);
        sibling_index = SIBLING(curr_index);

        // If the entry is a deletion entry and our sibling is set, do not propogate remove changes up
        // the tree because the parent is not free in this case
        if (!curr_entry && _al_btree_get_array_entry64(btree, sibling_index, 1)) {
            return;   
        }

        if (curr_entry && _al_btree_get_array_entry64(btree, parent_index, 1)) {
            return;   
        }

        _al_btree_set_array_entry64(btree, parent_index, curr_entry, 1);

        if (scan && curr_entry) {
            scan->level_count[i - 1] = 1;
        } else if (scan) {
            scan->level_count[i - 1] = -1;
        }

        curr_index = parent_index;
    }
}

int _al_btree_find_free_node(al_btree_t * btree, unsigned int level)
{   
    unsigned int index, curr_index, end_index;
    uint64_t level_mask = level_to_mask[level];

    if (al_btree_level_is_full(btree, level)) {
        return AL_BTREE_NULL_INDEX;
    }

    end_index = level_to_index[level + 1];
    for (curr_index = level_to_index[level]; curr_index < end_index; curr_index++) {
        if (!_al_btree_get_array_entry64(btree, curr_index, 1)) {
            return curr_index;
        }
    }

    return AL_BTREE_NULL_INDEX;
}

int al_btree_remove_node(al_btree_t * btree, al_btree_scan_t  * scan, unsigned int index)
{
    unsigned int level;

    if (!btree || index < 0 || index > AL_BTREE_NUM_ENTRIES) {
        ASSERT(0);
        return 1;
    }

    level = al_btree_index_to_level(index);

    _al_btree_add_array_entry(btree, scan, index, level, 0);

    return 0;
}

/* Add an entry to the Array list Binary tree
 */
int al_btree_add_node(al_btree_t * btree, al_btree_scan_t * scan, int level)
{
    int ret = 0;
    int index = 0;
    al_btree_entry_t default_entry;
    unsigned int num_entries;
    
    if (!btree || level < -1 || level > AL_BTREE_MAX_LEVEL) {
        ASSERT(0);
        return 1;
    }

    // If -1 is given then we assume we want the leaves of the tree
    if (level == -1)
        level = AL_BTREE_MAX_LEVEL;

    index = _al_btree_find_free_node(btree, level);
    if (index < 0) {
        return AL_BTREE_NULL_INDEX;
    }

    _al_btree_add_array_entry(btree, scan, index, level, 1);

    return index;
}
