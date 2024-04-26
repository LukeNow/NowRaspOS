#include <stddef.h>
#include <stdint.h>
#include <common/arraylist_btree.h>
#include <common/common.h>
#include <common/assert.h>
#include <common/math.h>
#include <common/string.h>

int al_btree_init(al_btree_t * btree, void * buffer, size_t num_entries)
{
    if (!btree || !num_entries || !buffer || !math_is_power2_64(num_entries)) {
        ASSERT(0);
        return 1;
    }
    
    btree->array = buffer;
    btree->num_entries = num_entries;
    btree->array_size = (AL_BTREE_ENTRY_SIZE * num_entries) / BITS_PER_BYTE; //In Bytes 

    return 0;
}

al_btree_entry_t _al_btree_get_array_entry(al_btree_t * btree, unsigned int index)
{
    unsigned int array_index = ENTRY_ARRAY_INDEX(index);
    unsigned int bit_index = 0;
    uint8_t mask = 0;
    uint8_t entry_data = btree->array[array_index];

    bit_index = (AL_BTREE_ENTRY_SIZE * index) % BITS_PER_BYTE;
    mask = BITS(AL_BTREE_ENTRY_SIZE) << bit_index;
    entry_data = (entry_data & mask) >> bit_index;

    return entry_data;
}

int _al_btree_set_array_entry(al_btree_t * btree, unsigned int index, al_btree_entry_t entry)
{
    unsigned int array_index = ENTRY_ARRAY_INDEX(index);
    unsigned int bit_index = 0;
    uint8_t mask = 0;
    uint8_t entry_data = btree->array[array_index];

    bit_index = (AL_BTREE_ENTRY_SIZE * index) % BITS_PER_BYTE;
    mask = BITS(AL_BTREE_ENTRY_SIZE) << bit_index;
    // Clear the entry, then set the entry with the data
    entry_data = (entry_data &~ mask) | (entry << bit_index);

    btree->array[array_index] = entry_data;
 
    return 0;
}

int _al_btree_add_array_entry(al_btree_t * btree, unsigned int index, al_btree_entry_t entry)
{
    unsigned int left_index;
    al_btree_entry_t left_entry;
    unsigned int right_index;
    al_btree_entry_t right_entry;
    unsigned int parent_index;
    al_btree_entry_t parent_entry;
    
    unsigned int curr_index;
    
    al_btree_entry_t entry_data = 0;
    int ret = 0;

    if (!btree) {
        ASSERT(0);
        return AL_NULL_INDEX;
    }

    _al_btree_set_array_entry(btree, index, entry);

    curr_index = index;

    while (curr_index) {
        parent_index = PARENT(curr_index);
        left_index = LEFT_CHILD(parent_index);
        right_index = RIGHT_CHILD(parent_index);

        left_entry = _al_btree_get_array_entry(btree, left_index);
        right_entry = _al_btree_get_array_entry(btree, right_index);
        parent_entry = _al_btree_get_array_entry(btree, parent_index);

        if ((right_entry == AL_ENTRY_SPLIT || left_entry == AL_ENTRY_SPLIT) ||
            right_entry == AL_ENTRY_FULL ^ left_entry == AL_ENTRY_FULL) {
            entry_data = AL_ENTRY_SPLIT;
        } else if (right_entry == AL_ENTRY_FULL && left_entry == AL_ENTRY_FULL) {
            entry_data = AL_ENTRY_FULL;
        }
        else {
            entry_data = AL_ENTRY_UNUSED;
        }

        _al_btree_set_array_entry(btree, parent_index, entry_data);
     
        curr_index = parent_index;
    }
    
    return ret;
}


int _al_btree_find_free_node(al_btree_t * btree, int level)
{
    unsigned int left_index;
    al_btree_entry_t left_entry;
    unsigned int right_index;
    al_btree_entry_t right_entry;
    unsigned int root_index;
    al_btree_entry_t root_entry;

    unsigned int curr_level = 0;
    unsigned int curr_index = 0;
    int ret = 0;
    
    if (!btree || level < -1) {
        ASSERT(0);
        return -1;
    }

    root_entry = _al_btree_get_array_entry(btree, 0);

    if (root_entry == AL_ENTRY_FULL) {
        return AL_NULL_INDEX; // There are no free leaves since root node is non-zero
    }

    while (curr_level != level) {
        left_index = LEFT_CHILD(curr_index);
        right_index = RIGHT_CHILD(curr_index);

        left_entry = _al_btree_get_array_entry(btree, left_index);
        
        right_entry = _al_btree_get_array_entry(btree, right_index);

        if (right_entry == AL_ENTRY_FULL && left_entry == AL_ENTRY_FULL) {
            return AL_NULL_INDEX;
        }

        if (left_entry == AL_ENTRY_SPLIT || left_entry == AL_ENTRY_UNUSED)
            curr_index = left_index;
        else if (right_entry == AL_ENTRY_SPLIT || right_entry == AL_ENTRY_UNUSED)
            curr_index = right_index;

        ASSERT(curr_index != PARENT(left_index));

        curr_level ++;
    }

    return curr_index;
}

int al_btree_is_full(al_btree_t * btree)
{
    al_btree_entry_t entry;

     if (!btree) {
        ASSERT(0);
        return -1;
    }

    entry = _al_btree_get_array_entry(btree, 0);

    if (entry == AL_ENTRY_FULL)
        return 1;

    return 0;
}

int al_btree_remove_node(al_btree_t * btree, unsigned int index)
{
    int ret = 0;

    if (!btree || index > -1) {
        ASSERT(0);
        return 1;
    }

    return _al_btree_add_array_entry(btree, index, AL_ENTRY_UNUSED);
}

/* Add an entry to the Array list Binary tree
 * Entry can be transient, but entry->data should not be
 * 
 */
int al_btree_add_node(al_btree_t * btree, int level)
{
    int ret = 0;
    int index = 0;
    
    if (!btree || level < -1) {
        ASSERT(0);
        return 1;
    }

    if (level == -1) { // -1 means finding the leaf node at the bottom level of btree
        level = math_log2_64(btree->num_entries) - 1;
    }

    index = _al_btree_find_free_node(btree, level);
    if (index < 0) {
        return AL_NULL_INDEX;
    }

    ret = _al_btree_add_array_entry(btree, index, AL_ENTRY_FULL);

    return index;
}
