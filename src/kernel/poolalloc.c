#include <stdint.h>
#include <stddef.h>
#include <kernel/poolalloc.h>
#include <common/assert.h>
#include <kernel/atomic.h>
#include <common/arraylist_btree.h>
#include <common/string.h>

size_t poolalloc_bucket_size(unsigned int num_objects)
{
    return POOLALLOC_NUM_BUCKETS(num_objects) * sizeof(poolalloc_bucket_t);
}


int poolalloc_init(poolalloc_t * pool, unsigned int num_objects, size_t object_size, void * object_buffer, void * bucket_array)
{
    if (!pool || !num_objects || !object_size || !object_buffer || !bucket_array) {
        ASSERT(0);
        return 1;
    }

    pool->buckets = bucket_array;
    pool->object_buffer = object_buffer;
    pool->num_buckets = POOLALLOC_NUM_BUCKETS(num_objects);
    pool->num_objects = num_objects;
    pool->object_size = object_size;

    memset(object_buffer, 0, object_size * num_objects);

    for (int i = 0; i < pool->num_buckets; i++) {
        memset(&pool->buckets[i], 0, sizeof(poolalloc_bucket_t));
        al_btree_init(&pool->buckets[i].btree);
    }

    return 0;
}

void * poolalloc_alloc(poolalloc_t * pool)
{
    poolalloc_bucket_t * bucket;
    unsigned int bucket_index;
    unsigned int entry_index = AL_BTREE_NULL_INDEX;
    al_btree_scan_t scan;
    
    if (!pool || !pool->object_buffer) {
        ASSERT(0);
        return NULL;
    }

    for (bucket_index = 0; bucket_index < pool->num_buckets; bucket_index++) {
        bucket = &pool->buckets[bucket_index];

        // Bucket is full, find another
        if (al_btree_level_is_full(&bucket->btree, AL_BTREE_MAX_LEVEL)) {
            continue;
        }

        entry_index = al_btree_atomic_add_node(bucket, &scan, -1);
        if (entry_index != AL_BTREE_NULL_INDEX) {
            break;
        }
    }

    // Buckets are all full most likely. Expand?
    if (entry_index == AL_BTREE_NULL_INDEX) {
        return NULL;
    }

    ASSERT(entry_index >= POOLALLOC_MIN_ARRAY_INDEX);

    return ((uint8_t*)(pool->object_buffer)) +  pool->object_size * ((bucket_index * POOLALLOC_OBJECTS_PER_BUCKET) + POOLALLOC_AL_INDEX_TO_ARRAY_INDEX(entry_index));
}


int poolalloc_free(poolalloc_t * pool, void * object)
{   
    int ret = 0;
    unsigned int bucket_index;
    unsigned int entry_index;
    uint64_t pool_ptr;
    al_btree_scan_t scan;

    if (!pool || !pool->object_buffer || !object) {
        ASSERT(0);
        return 1;
    }

    pool_ptr = (uint64_t)(object - pool->object_buffer);
    bucket_index = pool_ptr / (pool->object_size * POOLALLOC_OBJECTS_PER_BUCKET);

    ASSERT(bucket_index < pool->num_buckets);

    entry_index = (pool_ptr - (bucket_index * POOLALLOC_OBJECTS_PER_BUCKET * pool->object_size)) / pool->object_size;
    entry_index = POOLALLOC_ARRAY_INDEX_TO_AL_INDEX(entry_index);

    ASSERT(entry_index < POOLALLOC_ARRAY_INDEX_TO_AL_INDEX(POOLALLOC_OBJECTS_PER_BUCKET));

    return al_btree_atomic_remove_node(&pool->buckets[bucket_index], &scan, entry_index);
}
