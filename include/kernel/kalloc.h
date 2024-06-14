#ifndef __KALLOC_H
#define __KALLOC_H

#include <stddef.h>
#include <stdint.h>
#include <common/common.h>
#include <common/linkedlist.h>
#include <common/arraylist_btree.h>
#include <kernel/poolalloc.h>
#include <kernel/kern_defs.h>
#include <kernel/lock.h>
#include <common/bits.h>


/* INIT DEFS */
#define KALLOC_START_SMALL_BUCKETS 1
#define KALLOC_EARLYPAGE_DS_NUM 8

/* MISC DEFS */
#define KALLOC_BUCKET_AL_LEVEL_NUM AL_BTREE_LEVEL_NUM //There are 6 levels in total
#define KALLOC_BUCKET_MAX_AL_LEVEL AL_BTREE_MAX_LEVEL
#define KALLOC_BUCKET_LEVEL_NUM (KALLOC_BUCKET_AL_LEVEL_NUM * 2)
#define KALLOC_BUCKET_LEVEL_MAX ((KALLOC_BUCKET_MAX_AL_LEVEL * 2) + 1) //The "unified" bucket level scheme
#define KALLOC_RETRY_NUM 2
#define KALLOC_MAGIC 0xdeadbeef

/* PAGE BUCKET DEFS */
#define KALLOC_PAGE_BUCKET_MAX_BLOCK_SIZE 131072
#define KALLOC_PAGE_BUCKET_MIN_BLOCK_SIZE PAGE_SIZE
#define KALLOC_PAGE_BUCKETS_PER_MEMNODE 32 //The number of big buckets that fit in every mem node we assign
// Size tag that identifies the page size so we can keep track of alloc sizes in the memnode at a resonable size cost
#define KALLOC_PAGE_BUCKET_SIZE_TAG_AL_LEVEL 4
#define KALLOC_PAGE_BUCKET_SIZE_TAGS_PER_BUCKET (AL_BTREE_LEVEL_ENTRIES_NUM(KALLOC_PAGE_BUCKET_SIZE_TAG_AL_LEVEL))
#define KALLOC_PAGE_BUCKET_SIZE_TAGS_NUM (KALLOC_PAGE_BUCKETS_PER_MEMNODE * KALLOC_PAGE_BUCKET_SIZE_TAGS_PER_BUCKET)
#define KALLOC_PAGES_PER_PAGE_BUCKET_MAX_BLOCK (KALLOC_PAGE_BUCKET_MAX_BLOCK_SIZE / PAGE_SIZE)

/* MEMNODE DEFS */
#define KALLOC_MEMNODE_REGION_SIZE (KALLOC_PAGE_BUCKET_MAX_BLOCK_SIZE * KALLOC_PAGE_BUCKETS_PER_MEMNODE)
#define KALLOC_PAGES_PER_MEMNODE (KALLOC_PAGES_PER_PAGE_BUCKET_MAX_BLOCK * KALLOC_PAGE_BUCKETS_PER_MEMNODE)

/* SMALL BUCKET DEFS */
#define KALLOC_SMALL_BUCKET_MAX_BLOCK_SIZE 2048
#define KALLOC_SMALL_BUCKET_MIN_BLOCK_SIZE 64
#define KALLOC_SMALL_MAX_BLOCK_SIZES_PER_PAGE (PAGE_SIZE / KALLOC_SMALL_BUCKET_MAX_BLOCK_SIZE)
#define KALLOC_PAGES_PER_SMALL_BUCKET 32
#define KALLOC_SMALL_BUCKETS_PER_PAGE_BUCKET (KALLOC_PAGES_PER_SMALL_BUCKET * KALLOC_SMALL_MAX_BLOCK_SIZES_PER_PAGE) //64
#define KALLOC_SMALL_BUCKET_MIN_BLOCKS_PER_SMALL_BUCKET (KALLOC_SMALL_BUCKETS_PER_PAGE_BUCKET / KALLOC_SMALL_BUCKET_MIN_BLOCK_SIZE)
#define KALLOC_SMALL_BUCKET_LEVEL_START 6
#define KALLOC_SMALL_BUCKET_MEM_REGION_SIZE (KALLOC_PAGE_BUCKET_MAX_BLOCK_SIZE)

/***
 * kalloc.c is a buddy allocator that mainly uses a 64 bit arraylist binary-tree to implement a buddy allocator.
 * 
 * The main goal is to create a decent allocator that minimizes fragmentation and with a good amount of chunk sizes
 * while also trying to keep it as scaleable as possble for SMP i.e. trying to lock things in as little places as possible while
 * keeping things sane and consistent.
 * 
 * The main datastructure used is the common/arraylist_btree.c, which uses a 64bit array list binary tree with bits as the entry to give you 63 total entries.
 * This is nice because its compact and can easily be commited via cmpxchg to create atomic allocations of a binary tree without much need for locking. 
 * Although this allocation is best effort and is subject to stalling when under contention by other threads. The datastructure is also nice because it gives you 4
 * very bounded operations and searches. 
 * 
 * There is a best-effort caching of btrees (called buckets here) indexs for the appropriate type of alloc to help avoid the linear search of a free bucket, 
 * but must still be done in the case that the cached indexs are stale. Possibly more things can be done here but its good enough for now.
 * 
 * MY_NOTES: 
 * 
 * There is still the underflow possibility on the free_count updates when two threads free and alloc out of sync.
 */

/*
 * Struct for Small_bucket memory region.
 * We assign these memory regions from the memnode's page buckets
 * i.e. its a subset of the memnodes memory region.
 */
typedef struct kalloc_small_bucket {
    /* Magic value used to verify that the small_bucket is valid.
     * For sanities sake. Will remove later when the data structure allocation is
     * more locked down. */
    uint32_t magic;
    /* List of small_buckets in this memnode. */
    ll_node_t small_bucket_node;
    /* Memory region this small_bucket starts allocating at. */
    uint8_t * mem_region;
    /* Similar purpose to memnode's best effort index caching. */
    int free_buckets[KALLOC_BUCKET_AL_LEVEL_NUM];
    /* Ref count of nodes of this small bucket. */
    uint64_t free_count[KALLOC_BUCKET_AL_LEVEL_NUM];
    /* The array_list binary tree structure used to implement buddy allocation. */
    al_btree_t buckets[KALLOC_SMALL_BUCKETS_PER_PAGE_BUCKET];
} kalloc_small_bucket_t;

/*
 * Struct for a chunk of the memory allocats memory region. 
 */
typedef struct kalloc_memnode {
    /* List of small_buckets belonging to this memnode. */
    ll_node_t root_small_buckets_node;
    /* If we are expanding the small_buckets we need to lock it. */
    spinlock_t small_buckets_lock;
    /* List of memnodes chained together. */
    ll_node_t memnode_node;
    /* Memory region this memnode represents. */
    uint8_t * mem_region;
    /* Memnode index in list of memnodes. */
    unsigned int memnode_id;
    /* Ref count list of capacities for all nodes assigned to this memnode.
     * Is checked with mempressure limits for when to trigger expands. */
    uint64_t free_count[KALLOC_BUCKET_LEVEL_NUM];
    /* Represents a best-effort caching mechanism for indexs of the free buckets at
     * a given level. */
    int free_buckets[KALLOC_BUCKET_AL_LEVEL_NUM];
    /* The array_list binary tree structure used to implement buddy allocation. */
    al_btree_t page_buckets[KALLOC_PAGE_BUCKETS_PER_MEMNODE];
    /* Page bucket tags are the identifier for the size of page alloc
     * i.e. what al_level they were alloced at.
     * They are on a 16 tags basis per bucket, this is the minimum number of
     * tags we need to infer the size of an address that falls on that tag boundary. */
    uint8_t page_bucket_size_tags[KALLOC_PAGE_BUCKET_SIZE_TAGS_NUM];
} kalloc_memnode_t;

/*
 * Kalloc header embedded in small_allocs to verify alloc and get the size.
 * Risky that can be overwritten but for the overhead of small_allocs it seems worth it.
 */
typedef struct kalloc_header {
    uint32_t magic;
    size_t size;
} kalloc_header_t __attribute__ ((aligned (8)));;

/* Functions exported for testing */
void kalloc_get_total_allocs(uint64_t * totals, uint64_t * total_maxs, uint64_t * small_bucket_totals);
kalloc_memnode_t * kalloc_get_memnode_from_addr(uint8_t * addr);
kalloc_small_bucket_t * kalloc_get_small_bucket_from_addr(kalloc_memnode_t * memnode, uint8_t * addr);
size_t kalloc_size_to_alloc_size(size_t size);
unsigned int kalloc_size_to_bucket_level(size_t size);
unsigned int kalloc_get_al_level_from_addr(uint8_t * addr);
size_t kalloc_bucket_level_to_size(unsigned int level);


int kalloc_init(uint8_t * mem_region);
void * kalloc_alloc(size_t size, flags_t flags);
int kalloc_free(void * object, flags_t flags);

#endif