#include <stddef.h>
#include <stdint.h>
#include <common/arraylist_btree.h>
#include <common/common.h>
#include <common/assert.h>
#include <common/math.h>
#include <common/string.h>

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
#define TREE_FULL 0x7FFFFFFFFFFFFFFF


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


static inline unsigned int _al_btree_index_to_level(unsigned int index)
{   
    unsigned int i;
    for (i = 0; i < AL_BTREE_MAX_LEVEL + 1; i++) {
        if (index < level_to_index[i + 1])
            break;
    }

    return i;
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

unsigned int al_btree_level_is_full(al_btree_t * btree, unsigned int level)
{
    uint64_t level_mask = level_to_mask[level];
    
    return (btree->list & level_mask) == level_mask;
}

void _al_btree_add_array_entry(al_btree_t * btree, unsigned int index, unsigned int level, al_btree_entry_t entry)
{

    unsigned int child_index, parent_index, curr_index, sibling_index, curr_level, curr_size, curr_entry;
    uint64_t level_mask;
    
    ASSERT(btree && index < AL_BTREE_NUM_ENTRIES);    

    _al_btree_set_array_entry64(&btree->list, index, entry, 1);

    // Go down the tree and set all children of the node so that they cannot be reserved
    curr_level = level;
    curr_index = index;
    curr_size = 2;

    while (curr_level != AL_BTREE_MAX_LEVEL) {
        curr_entry = (entry ? BITS(curr_size) : 0);
        child_index = DEFAULT_CHILD(curr_index);
        _al_btree_set_array_entry64(&btree->list, child_index, curr_entry, curr_size);
        curr_level++;
        curr_index = child_index;
        curr_size = curr_size << 1;
    }
    

    //Go up the tree and set the parents that they are also unavailable. However if there are still free children that can be allocated and will be free
    curr_level = level;
    curr_index = index;
    curr_size = 1;

    while (curr_level != 0) {
        curr_entry = (entry ? BITS(curr_size) : 0);
        parent_index = PARENT(curr_index);
        sibling_index = SIBLING(curr_index);

        // If the entry is a deletion entry and our sibling is set, do not propogate remove changes up
        // the tree because the parent is not free in this case
        if (!curr_entry && _al_btree_get_array_entry64(&btree->list, sibling_index, 1)) {
            return;   
        }

        _al_btree_set_array_entry64(&btree->list, parent_index, curr_entry, 1);

        curr_level--;
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
        if (!_al_btree_get_array_entry64(&btree->list, curr_index, 1)) {
            return curr_index;
        }
    }

    return AL_BTREE_NULL_INDEX;
}

int al_btree_remove_node(al_btree_t * btree, unsigned int index)
{
    unsigned int level;

    if (!btree || index < 0 || index > AL_BTREE_NUM_ENTRIES) {
        ASSERT(0);
        return 1;
    }

    level = _al_btree_index_to_level(index);

    _al_btree_add_array_entry(btree, index, level, 0);

    return 0;
}

/* Add an entry to the Array list Binary tree
 */
int al_btree_add_node(al_btree_t * btree, int level)
{
    int ret = 0;
    int index = 0;
    al_btree_entry_t default_entry;
    unsigned int num_entries;
    
    if (!btree || level < -1 || level > AL_BTREE_MAX_LEVEL) {
        ASSERT(0);
        return 1;
    }

    // Special case of the root, check it and assign here
    if (level == 0 && al_btree_level_is_full(btree, 0)) {
       return AL_BTREE_NULL_INDEX;
    } else if (level == 0)  {
        btree->list = TREE_FULL;
        return 0;
    }

    if (level == -1)
        level = AL_BTREE_MAX_LEVEL;

    index = _al_btree_find_free_node(btree, level);
    if (index < 0) {
        return AL_BTREE_NULL_INDEX;
    }

    DEBUG_DATA_DIGIT("Found node=", index);
    _al_btree_add_array_entry(btree, index, level, 1);

    return index;
}
