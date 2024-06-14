#include <stddef.h>
#include <stdint.h>
#include <common/common.h>
#include <common/assert.h>
#include <common/arraylist_btree.h>
#include <common/linkedlist.h>
#include <kernel/kalloc.h>
#include <kernel/poolalloc.h>
#include <kernel/mm.h>
#include <kernel/kern_defs.h>
#include <kernel/lock.h>
#include <kernel/atomic.h>
#include <common/string.h>
#include <common/bits.h>

static const size_t page_bucket_sizes[] = {131072, 65536, 32768, 16384, 8192, 4096};
static const size_t small_bucket_sizes[] = {2048, 1024, 512, 256, 128, 64};
static const size_t bucket_sizes[] = {131072, 65536, 32768, 16384, 8192, 4096, 2048, 1024, 512, 256, 128, 64};

// Arbitrary amount available nodes at a given level that are based on ~vibes~ but probably should be around 10% left especially for the more heavily requested sizes
//static const unsigned int mempressure_limit[] = {4, 4, 4, 8, 8, 8, 2, 4, 4, 4, 8, 8};
static const unsigned int mempressure_limit[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

uint8_t * mem_ds_start;
uint8_t * mem_ds_end;
uint8_t * mem_ds_curr;
uint8_t * mem_start;
uint8_t * mem_end;
uint8_t * mem_curr;

static unsigned int memnode_count;
DEFINE_LOCK(root_mem_node_lock);
LL_ROOT_INIT(root_mem_node);

static unsigned int kalloc_bucket_level_to_al_level(unsigned int bucket_level)
{
    ASSERT_PANIC(bucket_level <= KALLOC_BUCKET_LEVEL_MAX, "Max level exceeded");

    return (bucket_level >= KALLOC_SMALL_BUCKET_LEVEL_START ? bucket_level - KALLOC_SMALL_BUCKET_LEVEL_START : bucket_level);
}

unsigned int kalloc_size_to_bucket_level(size_t size)
{   
    unsigned int i;
    for (i = 0; i < KALLOC_BUCKET_LEVEL_MAX; i++) {
        if (size > bucket_sizes[i + 1])
            return i;
    }

    return KALLOC_BUCKET_LEVEL_MAX;
}

static unsigned int kalloc_size_to_al_level(size_t size)
{
    unsigned int bucket_level = kalloc_size_to_bucket_level(size);

    return (size >= KALLOC_PAGE_BUCKET_MIN_BLOCK_SIZE ? bucket_level : bucket_level - KALLOC_SMALL_BUCKET_LEVEL_START);
}


size_t kalloc_size_to_alloc_size(size_t size)
{   
    size_t size_with_header = size + sizeof(kalloc_header_t);
    if (size_with_header > KALLOC_SMALL_BUCKET_MAX_BLOCK_SIZE && size <= KALLOC_SMALL_BUCKET_MAX_BLOCK_SIZE)
        return KALLOC_PAGE_BUCKET_MIN_BLOCK_SIZE;
    else if (size_with_header <= KALLOC_SMALL_BUCKET_MAX_BLOCK_SIZE)
        return size_with_header;
    else 
        return size;
}

static inline size_t kalloc_bucket_al_index_to_size_offset(unsigned int index, unsigned int bucket_level)
{
    size_t size = bucket_sizes[bucket_level];
    // The offset relative to the al_btree level
    unsigned int offset = al_btree_index_to_offset(index);

    return size * offset;
}

static size_t kalloc_bucket_al_level_to_small_size(unsigned int al_level)
{
    ASSERT_PANIC(al_level <= KALLOC_BUCKET_MAX_AL_LEVEL, "Max level exceeded");
    return small_bucket_sizes[al_level];
}

static size_t kalloc_bucket_al_level_to_page_size(unsigned int al_level)
{
    ASSERT_PANIC(al_level <= KALLOC_BUCKET_MAX_AL_LEVEL, "Max level exceeded");
    return page_bucket_sizes[al_level];
}

size_t kalloc_bucket_level_to_size(unsigned int bucket_level)
{   
    ASSERT_PANIC(bucket_level <= KALLOC_BUCKET_LEVEL_MAX, "Bucket max level exceeded");
    return bucket_sizes[bucket_level];
}

/** 
 * GET ADDR FUNCS 
 */
uint8_t * kalloc_get_small_bucket_addr(kalloc_small_bucket_t * small_bucket, unsigned int bucket_index, unsigned int assigned_index, unsigned int al_level)
{
    return (uint8_t *) (small_bucket->mem_region + (bucket_index * KALLOC_SMALL_BUCKET_MAX_BLOCK_SIZE) + kalloc_bucket_al_index_to_size_offset(assigned_index, al_level + KALLOC_SMALL_BUCKET_LEVEL_START));
}

uint8_t * kalloc_get_page_bucket_addr(kalloc_memnode_t * memnode, unsigned int bucket_index, unsigned int assigned_index, unsigned int al_level)
{
    return (uint8_t *) (memnode->mem_region + (bucket_index * KALLOC_PAGE_BUCKET_MAX_BLOCK_SIZE) + kalloc_bucket_al_index_to_size_offset(assigned_index, al_level));
}

kalloc_memnode_t * kalloc_get_memnode_from_addr(uint8_t * addr)
{
    kalloc_memnode_t * memnode;
    ll_node_t * node;

    LL_ITERATE_LIST(&root_mem_node, node) {
        memnode = (kalloc_memnode_t *)node->data;

        if ((uint64_t)addr >= (uint64_t)memnode->mem_region && (uint64_t)addr < ((uint64_t)memnode->mem_region + KALLOC_MEMNODE_REGION_SIZE))
            return memnode;
    }

    return NULL;
}

unsigned int kalloc_get_page_bucket_index_from_addr(kalloc_memnode_t * memnode, uint8_t * addr)
{
    for (int i = 0; i < KALLOC_PAGE_BUCKETS_PER_MEMNODE; i++) {
        uint64_t bucket_addr = (uint64_t)kalloc_get_page_bucket_addr(memnode, i, 0, 0);
        if ((uint64_t)addr >= bucket_addr && (uint64_t)addr < bucket_addr + KALLOC_PAGE_BUCKET_MAX_BLOCK_SIZE)
            return i;
    }

    return -1;
}

unsigned int kalloc_get_page_bucket_assigned_index_from_addr(kalloc_memnode_t * memnode, unsigned int bucket_index, unsigned int al_level, uint8_t * addr)
{
    size_t block_size = kalloc_bucket_al_level_to_page_size(al_level);
    unsigned int al_start_index = al_btree_level_to_index(al_level);
    unsigned int al_end_index = al_btree_level_to_index(al_level + 1);

    for (int i = al_start_index; i < al_end_index; i++) {
        uint64_t entry_addr = kalloc_get_page_bucket_addr(memnode, bucket_index, i, al_level);
        if ((uint64_t)addr >= entry_addr && (uint64_t)addr < (entry_addr) + block_size)
            return i;
    }

    return -1;
}

kalloc_small_bucket_t * kalloc_get_small_bucket_from_addr(kalloc_memnode_t * memnode, uint8_t * addr)
{
    ASSERT(memnode);
    ll_node_t * node;
    int i = 0;

    LL_ITERATE_LIST(&memnode->root_small_buckets_node, node) {
        kalloc_small_bucket_t * small_bucket = (kalloc_small_bucket_t *)node->data;
        ASSERT_PANIC(small_bucket->magic == KALLOC_MAGIC, "Small bucket magic invalid");

        if ((uint64_t)addr >= (uint64_t)small_bucket->mem_region && (uint64_t)addr < ((uint64_t)small_bucket->mem_region + KALLOC_SMALL_BUCKET_MEM_REGION_SIZE))
            return small_bucket;

        i++;
    }

    return NULL;
}

unsigned int kalloc_get_small_bucket_index_from_addr(kalloc_small_bucket_t * small_bucket, uint8_t * addr)
{
    for (int i = 0; i < KALLOC_SMALL_BUCKETS_PER_PAGE_BUCKET; i++) {
        uint64_t entry_addr = (uint64_t)kalloc_get_small_bucket_addr(small_bucket, i, 0, 0);
        if ((uint64_t)addr >= entry_addr && (uint64_t)addr < (entry_addr + KALLOC_SMALL_BUCKET_MAX_BLOCK_SIZE))
            return i;
    }

    return -1;
}

unsigned int kalloc_get_small_bucket_assigned_index_from_addr(kalloc_small_bucket_t * small_bucket, unsigned int bucket_index, unsigned int al_level, uint8_t * addr)
{
    size_t block_size = kalloc_bucket_al_level_to_small_size(al_level);
    unsigned int al_start_index = al_btree_level_to_index(al_level);
    unsigned int al_end_index = al_btree_level_to_index(al_level + 1);

    for (int i = al_start_index; i < al_end_index; i++) {
        uint64_t bucket_addr = (uint64_t)kalloc_get_small_bucket_addr(small_bucket, bucket_index, i, al_level);
        if ((uint64_t)addr >= bucket_addr && (uint64_t)addr < (bucket_addr + block_size))
            return i;
    }

    return -1;
}

/*
 * OTHER FUNCS
 */
static void _update_free_bucket(unsigned int al_level, unsigned int index, int * free_buckets, int free)
{
    int free_index = free_buckets[al_level];

    if (free && free_index == -1) {
        free_buckets[al_level] = index;
    } else if (!free && free_index == index) {
        free_buckets[al_level] = -1;
    }
}
static int kalloc_verify_update_free_bucket(al_btree_t * btree, unsigned int al_level, unsigned int index, int * free_buckets)
{   
    int free = !al_btree_level_is_full(btree, al_level);
    
    _update_free_bucket(al_level, index, free_buckets, free);

    return free;
}

int kalloc_update_free_buckets(al_btree_t * btree, unsigned int bucket_index, int * free_buckets)
{
    ASSERT(free_buckets && btree);
    
    for (int i = 0; i < KALLOC_BUCKET_AL_LEVEL_NUM; i++)
       kalloc_verify_update_free_bucket(btree, i, bucket_index, free_buckets);

    return 0;
}

static int kalloc_can_smallbucket_expand(kalloc_memnode_t * memnode)
{
    return !!memnode->free_count[0];
}

static int kalloc_memnode_bucket_level_at_mempressure_limit(kalloc_memnode_t * memnode, unsigned int bucket_level)
{   
    if (bucket_level >= KALLOC_SMALL_BUCKET_LEVEL_START) {
        // If we are searching for small buckets see if we are at capacity to allocate more small buckets
        return (memnode->free_count[bucket_level] <= mempressure_limit[bucket_level] && !kalloc_can_smallbucket_expand(memnode));
    } 

    return (memnode->free_count[bucket_level] <= mempressure_limit[bucket_level]);
}

static int kalloc_smallbucket_level_at_mempressure_limit(kalloc_small_bucket_t * small_bucket, unsigned int al_level)
{   
    return (small_bucket->free_count[al_level] <= mempressure_limit[al_level + KALLOC_SMALL_BUCKET_LEVEL_START]);
}

static kalloc_memnode_t * kalloc_get_free_memnode(unsigned int bucket_level)
{
    kalloc_memnode_t * memnode;
    ll_node_t * node;

    LL_ITERATE_LIST(&root_mem_node, node) {
        memnode = (kalloc_memnode_t *)node->data;
        unsigned int mem_pressured = kalloc_memnode_bucket_level_at_mempressure_limit(memnode, bucket_level);
        
        if (!mem_pressured) {
            return memnode;
        }

        if (bucket_level >= KALLOC_SMALL_BUCKET_LEVEL_START && mem_pressured && kalloc_can_smallbucket_expand(memnode)) {
            return memnode;
        }
    }

    return NULL;
}

static kalloc_small_bucket_t * kalloc_get_free_smallbucket_node(kalloc_memnode_t * memnode, unsigned int al_level)
{
    ASSERT(memnode);
    ll_node_t * node;

    LL_ITERATE_LIST(&memnode->root_small_buckets_node, node) {
        kalloc_small_bucket_t * small_bucket = (kalloc_small_bucket_t *)node->data;
        ASSERT_PANIC(small_bucket->magic == KALLOC_MAGIC, "Small bucket magic invalid");

        if (!kalloc_smallbucket_level_at_mempressure_limit(small_bucket, al_level))
            return small_bucket;
    }

    return NULL;
}

static unsigned int kalloc_get_free_page_bucket_index(kalloc_memnode_t * memnode, unsigned int al_level)
{
    ASSERT(memnode);

    int free_bucket_index = memnode->free_buckets[al_level];
    if (free_bucket_index != -1 && kalloc_verify_update_free_bucket(&memnode->page_buckets[free_bucket_index], al_level, free_bucket_index, memnode->free_buckets))
        return free_bucket_index;

    for (int i = 0; i < KALLOC_PAGE_BUCKETS_PER_MEMNODE; i++) {
        kalloc_update_free_buckets(&memnode->page_buckets[i], i, &memnode->free_buckets[0]);
        free_bucket_index = memnode->free_buckets[al_level];

        if (free_bucket_index != -1 && kalloc_verify_update_free_bucket(&memnode->page_buckets[free_bucket_index], al_level, free_bucket_index, memnode->free_buckets))
            return free_bucket_index;
    }

    DEBUG("No free level found");
    return -1;
}

static unsigned int kalloc_get_free_smallbucket_index(kalloc_small_bucket_t * small_bucket, unsigned int al_level)
{
    ASSERT(small_bucket);

    int free_bucket_index = small_bucket->free_buckets[al_level];
    if (free_bucket_index != -1 && kalloc_verify_update_free_bucket(&small_bucket->buckets[free_bucket_index], al_level, free_bucket_index, small_bucket->free_buckets))
        return free_bucket_index;
    
    for (int i = 0; i < KALLOC_SMALL_BUCKETS_PER_PAGE_BUCKET; i++) {
        kalloc_update_free_buckets(&small_bucket->buckets[i], i, &small_bucket->free_buckets[0]);

        free_bucket_index = small_bucket->free_buckets[al_level];
        if (free_bucket_index != -1 && kalloc_verify_update_free_bucket(&small_bucket->buckets[free_bucket_index], al_level, free_bucket_index, small_bucket->free_buckets))
            return free_bucket_index;
    }   

    DEBUG("No free level found");
    return -1;
}

// Assumes appropriate lock is held or called from safe context
static uint8_t * _get_ds_mem(size_t size)
{
    uint8_t *curr = mem_ds_curr;
    mem_ds_curr += size;
    //Round up to nearest 8 byte boundary just in case
    mem_ds_curr = (uint8_t*)ROUND_UP((uint64_t)mem_ds_curr, sizeof(uint64_t));

    ASSERT_PANIC(mem_ds_curr <= mem_ds_end, "MEM_DS MAX EXCEEDED");
    return curr;
}

static int kalloc_init_small_bucket(kalloc_small_bucket_t * small_bucket, kalloc_memnode_t * memnode, uint8_t * mem_region)
{
    ASSERT_PANIC(small_bucket && memnode && mem_region, "Init parameters invalid");
    
    memset(small_bucket, 0, sizeof(kalloc_small_bucket_t));
    
    small_bucket->magic = KALLOC_MAGIC;
    small_bucket->mem_region = mem_region;

    // Put the data for the mem_node we belong to in the ll node embedded in the small_bucket struct
    ll_node_init(&small_bucket->small_bucket_node, small_bucket);

    // Push the node embedded in the small_bucket struct for small_bucket lookups from the mem_node itself
    ll_push_list(&memnode->root_small_buckets_node, &small_bucket->small_bucket_node);

    // Assign free buckets for small bucket levels to the initialized small_bucket
    for (int i = 0; i < KALLOC_BUCKET_AL_LEVEL_NUM; i++) {
        small_bucket->free_buckets[i] = 0;
    }

    for (int i = 0; i < KALLOC_SMALL_BUCKETS_PER_PAGE_BUCKET; i++) {
        al_btree_init(&small_bucket->buckets[i]);
    }

    for (int i = 0; i < KALLOC_BUCKET_AL_LEVEL_NUM; i++) {
        uint64_t count = KALLOC_SMALL_BUCKETS_PER_PAGE_BUCKET << i;
        //Free counts for each level of the entire small bucket, increases by ^2 for each level
        small_bucket->free_count[i] = count;
        // Add the newly created nodes to the memnodes counts
        atomic_add(&memnode->free_count[i + KALLOC_SMALL_BUCKET_LEVEL_START], count);
    }

    return 0;
}

static int kalloc_init_memnode(kalloc_memnode_t *memnode, uint8_t *mem_region)
{
    int ret = 0;
    kalloc_small_bucket_t * small_bucket;

    ASSERT_PANIC(memnode && mem_region, "Init memnode parameters invalid");

    memset(memnode, 0, sizeof(kalloc_memnode_t));
    memnode->memnode_id = (mem_region - mem_start) / KALLOC_MEMNODE_REGION_SIZE;
    memnode->mem_region = mem_region;

    ll_root_init(&memnode->root_small_buckets_node);

    lock_init(&memnode->small_buckets_lock);

    ll_node_init(&memnode->memnode_node, memnode);
    // Add the first mem_node to the linked list
    // Needs to be locked access potentially for sanities sake
    if (ll_push_list(&root_mem_node, &memnode->memnode_node)) {
        ASSERT(0);
        DEBUG("!!!!node push list failed");
        return 1;
    }

    for (int i = 0; i < KALLOC_PAGE_BUCKETS_PER_MEMNODE; i++) {
        al_btree_init(&memnode->page_buckets[i]);
    }

    for (int i = 0; i < KALLOC_BUCKET_AL_LEVEL_NUM; i++) {
        // Assign the free indexs to the first bucket not taken by small bucket allocs
        memnode->free_buckets[i] = KALLOC_START_SMALL_BUCKETS;
        // Assign initial counts for the free nodes
        memnode->free_count[i] = KALLOC_PAGE_BUCKETS_PER_MEMNODE << i;
    }

    for (int i = 0; i < KALLOC_START_SMALL_BUCKETS; i++) {
        // Reserve the whole page bucket for small bucket and update count accordingly
        memnode->page_buckets[i] = AL_BTREE_FULL;
        for (int i = 0; i < KALLOC_BUCKET_AL_LEVEL_NUM; i++) {
            memnode->free_count[i] -= 1 << i;
        }

        small_bucket = _get_ds_mem(sizeof(kalloc_small_bucket_t));

        kalloc_init_small_bucket(small_bucket, memnode, memnode->mem_region + (size_t)(KALLOC_PAGE_BUCKET_MAX_BLOCK_SIZE * i));
    }

    memnode_count++;
    mem_end += KALLOC_MEMNODE_REGION_SIZE;

    return 0;
}

int kalloc_init(uint8_t * mem_region)
{
    int ret = 0;
    kalloc_memnode_t * memnode;
    size_t small_buckets_size;
    size_t big_buckets_size;
    uint8_t * node_pool_buckets;
    uint8_t * node_pool_buf;

    ASSERT_PANIC(mm_early_intialized() && mem_region, "Kalloc init parameters invalid");

    mem_ds_start = mm_earlypage_alloc(KALLOC_EARLYPAGE_DS_NUM);
    ASSERT_PANIC(mem_ds_start, "Early alloc failed");

    mem_ds_end = mem_ds_start + KALLOC_EARLYPAGE_DS_NUM * PAGE_SIZE;
    mem_ds_curr = mem_ds_start;

    mem_start = mem_region;

    // Will be incremented by kalloc_init_memnode
    mem_end = mem_region;

    ll_root_init(&root_mem_node);

    memnode = _get_ds_mem(sizeof(kalloc_memnode_t));
    kalloc_init_memnode(memnode, mem_region);

    return 0;
}

int kalloc_atomic_alloc(kalloc_memnode_t * memnode, kalloc_small_bucket_t * small_bucket, al_btree_t * btree, unsigned int al_level)
{
    al_btree_scan_t scan;
    int assigned_index;
    unsigned int node_level = 0;
    unsigned int level_offset = (small_bucket ? KALLOC_SMALL_BUCKET_LEVEL_START : 0);

    memset(&scan, 0, sizeof(al_btree_scan_t));
    assigned_index = al_btree_atomic_add_node(btree, &scan, al_level);

    if (assigned_index == AL_BTREE_NULL_INDEX)
        return assigned_index;

    //TODO There is still possibility of underflow when subtracting from the free_counts

    // Subtract a node above the one we assigned to if we filled a level above us
    for (int i = 0; i < al_level; i++) {
        ASSERT_PANIC(scan.level_count[i] != -1, "Alloc has released a node");
        // If there wasnt an alloc, do nothing
        if (scan.level_count[i] < 1)
            continue;

        ASSERT_PANIC(memnode->free_count[i + level_offset], "Decrementing count that is 0");
        atomic_sub(&memnode->free_count[i + level_offset], 1);
        // Do the same for the small bucket if applicable
        if (small_bucket) {
            ASSERT_PANIC(small_bucket->free_count[i], "Decrementing count that is 0");
            atomic_sub(&small_bucket->free_count[i], 1);
        }
    }

    // We are assigning all the nodes below the node we successfully allocated
    for (int i = al_level; i < KALLOC_BUCKET_AL_LEVEL_NUM; i++) {
        unsigned int node_count = 1 << node_level;
        ASSERT_PANIC(memnode->free_count[i + level_offset], "Decrementing count that is 0");
        ASSERT_PANIC(scan.level_count[i] > 0, "Alloc did free or no-op");
        atomic_sub(&memnode->free_count[i + level_offset], node_count);
        // Do the same for the small bucket if applicable
        if (small_bucket) {
            ASSERT_PANIC(small_bucket->free_count[i], "Decrementing count that is 0");
            atomic_sub(&small_bucket->free_count[i], node_count);
        }

        node_level ++;
    }

    return assigned_index;
}

int kalloc_atomic_free(kalloc_memnode_t * memnode, kalloc_small_bucket_t * small_bucket, al_btree_t * btree, unsigned int index)
{

    al_btree_scan_t scan;
    int ret;
    unsigned int node_level = 0;
    unsigned int level_offset = (small_bucket ? KALLOC_SMALL_BUCKET_LEVEL_START : 0);
    unsigned int al_level = al_btree_index_to_level(index);
    
    memset(&scan, 0, sizeof(al_btree_scan_t));
    ret = al_btree_atomic_remove_node(btree, &scan, index);

    if (ret)
        return ret;

    // add a node above the one we assigned to if we freed a node above us
    for (int i = 0; i < al_level; i++) {
        ASSERT_PANIC(scan.level_count[i] != 1, "Free has alloced a node");
        if (scan.level_count[i] > -1)
            continue;

        atomic_add(&memnode->free_count[i + level_offset], 1);

        // Do the same for the small bucket if applicable
        if (small_bucket)
            atomic_add(&small_bucket->free_count[i], 1);
    }

    // We are assigning all the nodes below the node we successfully allocated
    for (int i = al_level; i < KALLOC_BUCKET_AL_LEVEL_NUM; i++) {
        unsigned int node_count = 1 << node_level;

        ASSERT_PANIC(scan.level_count[i] < 0, "Free did alloc or no-op");

        atomic_add(&memnode->free_count[i + level_offset], node_count);
        // Do the same for the small bucket if applicable
        if (small_bucket)
            atomic_add(&small_bucket->free_count[i], node_count);
    
        node_level ++;
    }

    return 0;
}

int kalloc_expand_memnode(unsigned int bucket_level)
{   
    kalloc_memnode_t * memnode;
    int ret = 0;

    lock_spinlock(&root_mem_node_lock);
    if (!kalloc_memnode_bucket_level_at_mempressure_limit(memnode, bucket_level)) {
        goto memnode_expand_succeed;
    }

    memnode = _get_ds_mem(sizeof(kalloc_memnode_t));
    ret = kalloc_init_memnode(memnode, mem_end);
memnode_expand_succeed:
    lock_spinunlock(&root_mem_node_lock);
    return 0;

memnode_expand_fail:
    lock_spinunlock(&root_mem_node_lock);
    return 1;
}

int kalloc_expand_small_buckets(kalloc_memnode_t * memnode, unsigned int bucket_level)
{
    kalloc_small_bucket_t * small_bucket;
    int free_index;
    int assigned_index;
    int ret = 0;

    ASSERT_PANIC(memnode, "Memnode is null");

    lock_spinlock(&memnode->small_buckets_lock);

    if (!kalloc_can_smallbucket_expand(memnode)) {
        goto small_expand_fail;
    }

    free_index = kalloc_get_free_page_bucket_index(memnode, 0);
    if (free_index == -1) {
        goto small_expand_fail;
    }

    assigned_index = kalloc_atomic_alloc(memnode, NULL, &memnode->page_buckets[free_index], 0);
    if (assigned_index == -1) {
        goto small_expand_fail;
    }

    small_bucket = _get_ds_mem(sizeof(kalloc_small_bucket_t));

    ret = kalloc_init_small_bucket(small_bucket, memnode, memnode->mem_region + (KALLOC_PAGE_BUCKET_MAX_BLOCK_SIZE * free_index));
    ASSERT_PANIC(!ret, "Init small bucket failed");

small_expand_succeed:
    lock_spinunlock(&memnode->small_buckets_lock);
    return 0;

small_expand_fail:
    lock_spinunlock(&memnode->small_buckets_lock);
    return 1;
}

int kalloc_expand(kalloc_memnode_t * memnode, unsigned int bucket_level)
{     
    int ret = 0;

    if (!memnode) {
        return kalloc_expand_memnode(bucket_level);
    }

    if (kalloc_can_smallbucket_expand(memnode)) {
        ret = kalloc_expand_small_buckets(memnode, bucket_level);
    }

    if (!ret)
        return ret;

    return kalloc_expand_memnode(bucket_level);
}

// Get the totals as seen by the memnodes and smallbuckets vs the theoretical total_max free counts
void kalloc_get_total_allocs(uint64_t * totals, uint64_t * total_maxs, uint64_t * small_bucket_totals)
{
    kalloc_memnode_t * memnode;
    kalloc_small_bucket_t * small_bucket;
    ll_node_t * memnode_node;
    ll_node_t * smallbucket_node;
    unsigned int small_bucket_count = 0;
    int memnode_count = 0;

    LL_ITERATE_LIST(&root_mem_node, memnode_node) {
        memnode = (kalloc_memnode_t *)memnode_node->data;
        small_bucket_count = 0;

        LL_ITERATE_LIST(&memnode->root_small_buckets_node, smallbucket_node) {
            kalloc_small_bucket_t * small_bucket = (kalloc_small_bucket_t *)smallbucket_node->data;
            ASSERT_PANIC(small_bucket->magic == KALLOC_MAGIC, "Small bucket magic invalid");

            for (int i = 0; i < KALLOC_BUCKET_AL_LEVEL_NUM; i++) {
                //total_maxs[i + KALLOC_SMALL_BUCKET_LEVEL_START] += KALLOC_SMALL_BUCKETS_PER_PAGE_BUCKET << i;
                small_bucket_totals[i  + KALLOC_SMALL_BUCKET_LEVEL_START] += small_bucket->free_count[i];
            }

            small_bucket_count++;
        }
        
        unsigned int max_block_count = KALLOC_PAGE_BUCKETS_PER_MEMNODE - small_bucket_count;
        for (int i = 0; i < KALLOC_BUCKET_AL_LEVEL_NUM; i++) {
            total_maxs[i] += (max_block_count << i);
            totals[i] += memnode->free_count[i];
        }

        max_block_count = KALLOC_SMALL_BUCKETS_PER_PAGE_BUCKET * small_bucket_count;
        for (int i = KALLOC_SMALL_BUCKET_LEVEL_START; i < KALLOC_BUCKET_LEVEL_NUM; i++) {
            total_maxs[i] += max_block_count << (i - KALLOC_SMALL_BUCKET_LEVEL_START);
            totals[i] += memnode->free_count[i];
        }
        
        memnode_count ++;
    }
}

unsigned int _get_page_bucket_size_tag_from_addr(kalloc_memnode_t * memnode, uint8_t * addr)
{
    uint64_t tags_addr = (uint64_t)addr - (uint64_t)memnode->mem_region;
    unsigned int tags_index = tags_addr / kalloc_bucket_al_level_to_page_size(KALLOC_PAGE_BUCKET_SIZE_TAG_AL_LEVEL);
    ASSERT_PANIC(tags_index < KALLOC_PAGE_BUCKET_SIZE_TAGS_NUM, "Tags_index invalid");

    return memnode->page_bucket_size_tags[tags_index];
}

unsigned int _get_page_bucket_tags_index(unsigned int al_level, unsigned int bucket_index, unsigned int assigned_index)
{
    unsigned int tags_offset;
    unsigned int tags_index;

    if (al_level <= KALLOC_PAGE_BUCKET_SIZE_TAG_AL_LEVEL) {
        tags_offset = (1 << (KALLOC_PAGE_BUCKET_SIZE_TAG_AL_LEVEL - al_level))  * al_btree_index_to_offset(assigned_index);
    } else {
        tags_offset = al_btree_index_to_offset(assigned_index) / 2;
    }

    ASSERT_PANIC(tags_offset < 16, "TAGS OFFSET INVALID");
    tags_index = bucket_index * KALLOC_PAGE_BUCKET_SIZE_TAGS_PER_BUCKET + tags_offset;
    ASSERT_PANIC(tags_index < KALLOC_PAGE_BUCKET_SIZE_TAGS_NUM, "tags_invalid");
    
    return tags_index;
}

unsigned int kalloc_get_al_level_from_addr(uint8_t * addr)
{
    kalloc_memnode_t * memnode = kalloc_get_memnode_from_addr(addr);
    return _get_page_bucket_size_tag_from_addr(memnode, addr);
}

void * kalloc_alloc(size_t size, flags_t flags)
{
    size_t alloc_size;
    unsigned int al_bucket_level, bucket_level;
    int assigned_index, bucket_index;
    uint8_t * addr;
    al_btree_t * btree;
    kalloc_memnode_t * memnode;
    kalloc_small_bucket_t * small_bucket;
    int ret;
    unsigned int retry = 0;

    if (!size) {
        ASSERT(0);
        return NULL;
    }

    if (size > KALLOC_PAGE_BUCKET_MAX_BLOCK_SIZE) {
        ASSERT(0);
        DEBUG("Kalloc size greater than max page bucket size");
        return NULL;
    }
    
    alloc_size = kalloc_size_to_alloc_size(size);
    bucket_level = kalloc_size_to_bucket_level(alloc_size);
    al_bucket_level = kalloc_bucket_level_to_al_level(bucket_level);

retry_alloc:    
    ret = 0;
    memnode = NULL;
    small_bucket = NULL;

    if (retry == KALLOC_RETRY_NUM) {
        DEBUG("Kalloc retried and failed");
        return NULL;
    }

    memnode = kalloc_get_free_memnode(bucket_level);
    if (!memnode) {
        retry++;
        ret = kalloc_expand(memnode, bucket_level);
        goto retry_alloc;
    }

    if (alloc_size > KALLOC_SMALL_BUCKET_MAX_BLOCK_SIZE) {
        ASSERT_PANIC(bucket_level < KALLOC_SMALL_BUCKET_LEVEL_START, "Bucket level inconsistent");
        
        bucket_index = kalloc_get_free_page_bucket_index(memnode, al_bucket_level);
        if (bucket_index == -1) {
            retry++;
            goto retry_alloc;
        }

        btree = &memnode->page_buckets[bucket_index];
    } else {
        small_bucket = kalloc_get_free_smallbucket_node(memnode, al_bucket_level);
        if (!small_bucket) {
            ret = kalloc_expand(memnode, bucket_level);
            retry++;
            goto retry_alloc;
        }

        bucket_index = kalloc_get_free_smallbucket_index(small_bucket, al_bucket_level);
        if (bucket_index == -1) {
            retry++;
            goto retry_alloc;
        }

        btree = &small_bucket->buckets[bucket_index];
    }

    assigned_index = kalloc_atomic_alloc(memnode, small_bucket, btree, al_bucket_level);

    if (assigned_index == AL_BTREE_NULL_INDEX) {
        retry++;
        goto retry_alloc;
    }

    // We only do inplace headers for the small allocs because big allocs are on a page basis
    if (alloc_size < KALLOC_PAGE_BUCKET_MIN_BLOCK_SIZE) {
        addr = kalloc_get_small_bucket_addr(small_bucket, bucket_index, assigned_index, al_bucket_level);
        ((kalloc_header_t *)addr)->magic = KALLOC_MAGIC;
        ((kalloc_header_t *)addr)->size = alloc_size;
        addr += sizeof(kalloc_header_t);
    } else {
        unsigned int tags_index = _get_page_bucket_tags_index(al_bucket_level, bucket_index, assigned_index);
        memnode->page_bucket_size_tags[tags_index] = al_bucket_level;
        addr = kalloc_get_page_bucket_addr(memnode, bucket_index, assigned_index, al_bucket_level);
    }
    
    return addr;
}

int kalloc_free(void * object, flags_t flags)
{
    uint8_t * addr;
    size_t size;
    uint32_t magic;
    unsigned int bucket_level, al_bucket_level;
    int bucket_index, assigned_index;
    kalloc_memnode_t * memnode = NULL;
    kalloc_small_bucket_t * small_bucket = NULL;
    int ret = 0;
    al_btree_t * btree;

    if (!object) {
        ASSERT(0);
        DEBUG("Null object given to kalloc_free");
        return 1;
    }

    if (object < mem_start || object >= mem_end) {
        ASSERT(0);
        DEBUG("Bad object address given");
        return 1;
    }

    memnode = kalloc_get_memnode_from_addr(object);
    if (!memnode) {
        ASSERT_PANIC(0, "memnode could not be found from object");
        return 1;
    }

    small_bucket = kalloc_get_small_bucket_from_addr(memnode, object);
    if (small_bucket) {
        addr = (uint8_t *)object - sizeof(kalloc_header_t);
        magic = ((kalloc_header_t *)addr)->magic;
        size = ((kalloc_header_t *)addr)->size;

        ASSERT_PANIC(magic == KALLOC_MAGIC, "Kalloc magic invalid for free");
        ASSERT_PANIC(size <= KALLOC_SMALL_BUCKET_MAX_BLOCK_SIZE, "Kalloc invalid size found");

        bucket_level = kalloc_size_to_bucket_level(size);
        al_bucket_level = kalloc_bucket_level_to_al_level(bucket_level);

        bucket_index = kalloc_get_small_bucket_index_from_addr(small_bucket, addr);

        assigned_index = kalloc_get_small_bucket_assigned_index_from_addr(small_bucket, bucket_index, al_bucket_level, addr);

        btree = &small_bucket->buckets[bucket_index];
    } else {
        addr = (uint8_t *)object;
        al_bucket_level = _get_page_bucket_size_tag_from_addr(memnode, addr);
        ASSERT_PANIC(al_bucket_level < KALLOC_BUCKET_AL_LEVEL_NUM, "al_bucket_level from tag invalid");
        
        size = kalloc_bucket_al_level_to_page_size(al_bucket_level);
        bucket_index = kalloc_get_page_bucket_index_from_addr(memnode, addr);
        assigned_index = kalloc_get_page_bucket_assigned_index_from_addr(memnode, bucket_index, al_bucket_level, addr);
        btree = &memnode->page_buckets[bucket_index];
    }
    
    ret = kalloc_atomic_free(memnode, small_bucket, btree, assigned_index);
    if (ret) {
        ASSERT(0);
        DEBUG("kalloc free failed");
        return 1;
    }

    return 0;
}
