#ifndef __POOLALLOC_H
#define __POOLALLOC_H

#include <stddef.h>
#include <stdint.h>
#include <common/arraylist_btree.h>

#define POOLALLOC_OBJECTS_PER_BUCKET AL_BTREE_MAX_LEVEL_ENTRIES
#define POOLALLOC_MIN_ARRAY_INDEX AL_BTREE_MAX_LEVEL_START_INDEX
#define POOLALLOC_AL_INDEX_TO_ARRAY_INDEX(INDEX) ((INDEX) - POOLALLOC_MIN_ARRAY_INDEX)
#define POOLALLOC_ARRAY_INDEX_TO_AL_INDEX(INDEX) (AL_BTREE_MAX_LEVEL_START_INDEX + (INDEX))
#define POOLALLOC_NUM_BUCKETS(num_objects) (((num_objects) >= POOLALLOC_OBJECTS_PER_BUCKET) ? ((num_objects) / POOLALLOC_OBJECTS_PER_BUCKET) : 1)

typedef struct poolalloc_bucket {
    al_btree_t btree;
} poolalloc_bucket_t;

typedef struct poolalloc {
    poolalloc_bucket_t * buckets;
    unsigned int num_buckets;
    unsigned int num_objects;
    size_t object_size;
    void * object_buffer;
} poolalloc_t;

size_t poolalloc_bucket_size(unsigned int num_objects);
int poolalloc_init(poolalloc_t * pool, unsigned int num_objects, size_t object_size, void * object_buffer, void * bucket_buffer);
void * poolalloc_alloc(poolalloc_t * pool);
int poolalloc_free(poolalloc_t * pool, void * object);

#endif