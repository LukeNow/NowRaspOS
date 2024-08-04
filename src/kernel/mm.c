#include <stdint.h>
#include <stddef.h>
#include <common/assert.h>
#include <kernel/addr_defs.h>
#include <kernel/kern_defs.h>
#include <kernel/uart.h>
#include <kernel/lock.h>
#include <kernel/mm.h>
#include <kernel/kalloc_cache.h>
#include <common/math.h>

#define MM_INIT_NODE_CACHE_PAGES 1

DEFINE_SPINLOCK(mm_lock);
DEFINE_SPINLOCK(early_mm_lock);

LL_ROOT_INIT(mm_area_root_node);

static mm_global_area_t global_area;
static kalloc_cache_t mm_node_cache;

static int early_page_num = 0;
static uint8_t *early_page_start;
static uint8_t *early_page_end;
static uint8_t *early_page_curr;
static uint8_t *mm_data_top;

static int early_mm_initialized = 0;
static int mm_initialized = 0;

/* Early page/alloc functions. */

void _align_early_mem(size_t size)
{
    mm_data_top = ALIGN_UP((uint64_t)mm_data_top, size);
}

uint8_t * _early_data_alloc(size_t size)
{   
    uint8_t * top;

    ASSERT_PANIC(mm_data_top, "MM data top is NULL");
    
    top = mm_data_top;
    mm_data_top += size;
    _align_early_mem(sizeof(uint64_t));

    memset(top, 0, size);

    return top;
}

uint8_t *_early_page_data_alloc(unsigned int page_num)
{
    _align_early_mem(PAGE_SIZE);

    return _early_data_alloc(page_num * PAGE_SIZE);
}

int mm_early_is_intialized()
{
    return early_mm_initialized;
}

int mm_early_init()
{
    uint32_t early_page_size = ((uint64_t) __earlypage_end) - ((uint64_t) __earlypage_start);
    early_page_num = early_page_size >> PAGE_BIT_OFF;
    early_page_start = ((uint64_t) __earlypage_start);
    early_page_end = ((uint64_t) __earlypage_end);
    early_page_curr = early_page_start;

    mm_data_top = (char *)early_page_end;

    ASSERT(IS_ALIGNED((uint64_t)early_page_start, PAGE_SIZE));
    ASSERT(IS_ALIGNED((uint64_t)early_page_end, PAGE_SIZE));
    
    early_mm_initialized = 1;

    DEBUG("--- Early MM initialized ---");

    return 0;
}

void *mm_earlypage_alloc(int num_pages)
{
    ASSERT_PANIC(mm_early_is_intialized(), "MM early is not initialized.");

    lock_spinlock(&early_mm_lock);

    uint8_t *start = early_page_curr;
    uint8_t *curr = early_page_curr;

    for  (int i = 0; i < num_pages; i++)  {
        if (curr + PAGE_SIZE > early_page_end) {
            ASSERT(0);
            return NULL;
        }
        curr += PAGE_SIZE;
    }

    early_page_curr = curr;
    lock_spinunlock(&early_mm_lock);

    return start;
}

int mm_earlypage_shrink(int num_pages)
{
    ASSERT_PANIC(mm_early_is_intialized(), "MM early is not initialized.");

    lock_spinlock(&early_mm_lock);

    uint8_t *start = early_page_curr;
    uint8_t *curr = early_page_curr;

    for  (int i = 0; i < num_pages; i++)  {
        if (curr - PAGE_SIZE < early_page_start) {
            ASSERT(0);
            return 1;
        }
        curr -= PAGE_SIZE;
    }

    early_page_curr = curr;
    lock_spinunlock(&early_mm_lock);

    return 0;
}

/* MM page alloc functions. */

int mm_is_initialized()
{
    return mm_initialized;
}

/* Alloc ll_nodes we use for representing buddy nodes in our lists. */
ll_node_t * mm_alloc_node(void * data)
{   
    ll_node_t * node;
    void * new_page;
    kalloc_slab_t * slab;
    
    if (mm_node_cache.num == mm_node_cache.max_num)  {
        new_page = _early_page_data_alloc(1);
        slab = kalloc_cache_add_slab_pages(&mm_node_cache, new_page, 1);
        ASSERT_PANIC(slab, "MM node cache slab alloc failed");
    }

    node = kalloc_cache_alloc(&mm_node_cache);
    if (!node) {
        DEBUG_THROW("MM ALLOC NODE could not allocate from node cache");
        return NULL;
    }

    ll_node_init(node, data);

    return node;
}

/* Free ll_nodes we are using as buddy nodes. */
void mm_free_node(ll_node_t * node)
{
    ASSERT_PANIC(node, "ll node null");

    memset((void*)node, 0, sizeof(ll_node_t));

    if (kalloc_cache_free(&mm_node_cache, (void*)node))  {
        DEBUG_PANIC("Kalloc cache free for ll node failed");
    }
}

mm_global_area_t * mm_global_area()
{
    return &global_area;
}

int mm_page_is_valid(unsigned int page_index)
{
    return mm_global_area()->global_pages[page_index].page_flags & MM_PAGE_VALID;
}

void mm_mark_page_valid(unsigned int page_index)
{
    ASSERT_PANIC(!mm_page_is_valid(page_index), "MM double marking page valid.");
    mm_global_area()->global_pages[page_index].page_flags |= MM_PAGE_VALID;
}

void mm_mark_page_free(unsigned int page_index)
{
    ASSERT_PANIC(mm_page_is_valid(page_index), "MM double marking page free.");
    mm_global_area()->global_pages[page_index].page_flags &= ~MM_PAGE_VALID;
}

void mm_link_page_obj_ptr(unsigned int page_index, void * ptr)
{
    ASSERT_PANIC(mm_page_is_valid(page_index), "MM linking obj ptr to invalid page.");
    mm_global_area()->global_pages[page_index].kalloc_obj = ptr;
}

void * mm_get_page_obj_ptr(unsigned int page_index)
{
    return mm_global_area()->global_pages[page_index].kalloc_obj;
}

unsigned int mm_pages_to_memorder(unsigned int page_num)
{
    page_num = math_align_power2_64(page_num);

    return math_log2_64(page_num);
}

unsigned int mm_memorder_to_pages(unsigned int memorder)
{
    return 1 << memorder;
}

int mm_pages_are_free(uint64_t page_addr, unsigned int memorder)
{
    mm_page_t * pages = mm_global_area()->global_pages;
    unsigned int page_index = MM_MEMORDER_INDEX(page_addr, 0);
    unsigned int page_num = mm_memorder_to_pages(memorder);

    for (int i = 0; i < page_num; i++){
        // There is an active page in the range, return not free
        if(pages[page_index].page_flags & MM_PAGE_VALID) {
            return 0;
        }

        page_index++;
    }

    // Pages are free and not active
    return 1;
}

mm_area_t * mm_area_from_addr(uint64_t addr)
{
    unsigned int area_index;
    mm_global_area_t * global_area = mm_global_area();

    addr = ALIGN_DOWN(addr, MM_AREA_SIZE);
    area_index = MM_AREA_INDEX(addr);
    if (area_index >= global_area->area_count) {
        DEBUG("Area index from addr exceedes global areas");
        return NULL;
    }

    return &global_area->global_areas[area_index];
}

/* Find a area that has a free buddy at memorder level or higher. */
mm_area_t * _find_free_area(unsigned int memorder)
{   
    ll_node_t * node;
    mm_global_area_t * global_area = mm_global_area();
    for (unsigned int free_memorder = memorder; free_memorder < MM_MAX_ORDER + 1; free_memorder++) {
        node = MM_GLOBAL_AREA_FREE_AREA(free_memorder);
        if (node)
            return (mm_area_t*)node->data;
    }

    return NULL;
}

/* Get the memorder from the buddy index. Find the memorder from the most significant bit*/
unsigned int _memorder_from_buddy_index(unsigned int buddy_index)
{   
    unsigned int found_memorder = bits_msb_index_32(++buddy_index) - 1;
    ASSERT_PANIC(found_memorder <= MM_MAX_ORDER, "Found memorder exceeds range");

    return MM_MAX_ORDER - found_memorder;
}

/* This finds a free node at the given index or from a parent above the index. */
ll_node_t * _find_free_ancestor_node(mm_area_t * area, unsigned int buddy_index, unsigned int memorder)
{   
    ll_node_t * free_list;
    unsigned int curr_buddy_index = buddy_index;

    for (unsigned int i = memorder; i < MM_MAX_ORDER + 1; i++) {
        ll_node_t * free_node;
        
        free_list = &area->free_buddy_list[i];

        // TODO more efficiently find a buddy node, this is costly when 
        // the buddy nodes are all split and the free list is populated
        free_node = ll_search_data(free_list, (void*)curr_buddy_index);

        // If the node was found to be free, we return that either the buddy itself is free or one of its parents are free
        if (free_node)
            return free_node;

        curr_buddy_index = MM_BUDDY_PARENT(curr_buddy_index);
    }

    // We iterated through the free lists and could not find a parent in one of them
    return NULL;
}

/* This is checking if the buddy nodes 'buddy' space has a valid page. In the case of a "split" buddy node
 * this tells us which of the two memory spaces of the buddy node are free. */
int _buddy_node_buddy_is_free(mm_area_t * area, unsigned int buddy_index, unsigned int memorder)
{
    unsigned int buddy_offset = MM_BUDDY_PAGE_OFFSET(buddy_index, memorder);
    unsigned int page_index = area->start_page_index + buddy_offset;
    unsigned int memorder_pages = MM_MEMORDER_TO_PAGES(memorder);

    int left_free = 1;
    int right_free = 1;

    for (int i = 0; i < memorder_pages; i++) {
        if (mm_page_is_valid(page_index + i))
            left_free = 0;

        if (mm_page_is_valid(page_index + i + memorder_pages))
            right_free = 0;

    }

    return left_free;
}

/* Mark the buddy by flipping the bitmap bit it is associated with. */
unsigned int _mark_buddy(mm_area_t * area, unsigned int buddy_index)
{   
    bitmap_t * bitmap = area->bitmap;

    unsigned int buddy_status = bitmap_get(bitmap, buddy_index);
    bitmap_flip(bitmap, buddy_index);

    return buddy_status;
}

/* Add a buddy_node to the area->free_list as well as checking if the area should be added to
 * the global_area->free_list. */
ll_node_t * _add_buddy_list(mm_area_t * area, unsigned int memorder, unsigned int buddy_index)
{
    ll_node_t * buddy_node;
    ll_node_t * node;
    ll_node_t * global_areas_list = &mm_global_area()->free_areas_list[memorder];
    ll_node_t * buddy_list = &area->free_buddy_list[memorder];

    buddy_node = mm_alloc_node((void*)buddy_index);

    // If the area list was empty before, we push this area to the global free lists
    if (!MM_AREA_FREE_BUDDY(area, memorder)) {
        // TODO Enable this search in DEBUG only
        node = ll_search_data(global_areas_list, (void*)&area->global_area_nodes[memorder]);
        if (node) {
            DEBUG_PANIC("Found area already in global list, double add");
        }

        if (ll_push_list(global_areas_list, &area->global_area_nodes[memorder]))  {
            DEBUG_PANIC("Could not push global areas list");
            return NULL;
        }

        //DEBUG_FUNC("-Added area to global list, area node=", (uint64_t)area->global_area_nodes[memorder].data);
    }

    // TODO Enable this search in DEBUG only
    node = ll_search_data(buddy_list, buddy_index);
    if (node) {
        DEBUG_PANIC("Buddy index being double added to list");
    }

    if (ll_push_list(buddy_list, buddy_node))  {
        DEBUG_PANIC("LL node push list failed");
        return NULL;
    }

    //DEBUG_FUNC_DIGIT("-Added buddy node to area list. buddy_index=", (uint64_t)buddy_node->data);

    return buddy_node;
}

/* Remove from an area's->free_list and check if we need to remove an area from the 
 * global_areas->free_list. */
int _remove_buddy_list(mm_area_t * area, unsigned int memorder, ll_node_t * buddy_node)
{
    ll_node_t * global_areas_list = &mm_global_area()->free_areas_list[memorder];
    ll_node_t * buddy_list = &area->free_buddy_list[memorder];

    //DEBUG_FUNC_DIGIT("-Remove buddy node to area list. buddy_index=", (uint64_t)buddy_node->data);

    if (ll_delete_node(buddy_list, buddy_node))  {
        DEBUG_PANIC("Ll node delete fail");
        return 1;
    }
    mm_free_node(buddy_node);

    // If there are no more free buddys of this memorder in this area, remove it from the global areas free list
    if (!MM_AREA_FREE_BUDDY(area, memorder))  {
        if (ll_delete_node(global_areas_list, &area->global_area_nodes[memorder])) {
            DEBUG_PANIC("LL area list delete fail");
            return 1;
        }

        //DEBUG_FUNC("-Remove area to global list, area node=", (uint64_t)area->global_area_nodes[memorder].data);
    }

    return 0;
}

mm_buddy_dir_t _buddy_split_dir_from_addr(uint64_t buddy_addr, uint64_t target_addr, unsigned int memorder)
{
    unsigned int buddy_index = MM_MEMORDER_INDEX(buddy_addr, memorder);
    unsigned int pages = MM_MEMORDER_TO_PAGES(memorder);

    ASSERT_PANIC(VAL_IN_RANGE(target_addr, buddy_addr, (pages * PAGE_SIZE * 2)), "Buddy addr not in found range");
    
    if (VAL_IN_RANGE(target_addr, buddy_addr, (pages * PAGE_SIZE))) {
        return mm_buddy_left_child;
    }

    return mm_buddy_right_child;
}

/* Returns a valid split direction of the buddy and verifies the requested direction if given. */
mm_buddy_dir_t _buddy_split_dir(mm_area_t * area, ll_node_t * buddy_node, unsigned int buddy_status, mm_buddy_dir_t requested_dir)
{   
    mm_buddy_dir_t dir;
    unsigned int buddy_index = MM_BUDDY_INDEX(buddy_node);
    unsigned int memorder = _memorder_from_buddy_index(buddy_index);
    
    if (buddy_status) {
        unsigned int left_child_free = _buddy_node_buddy_is_free(area, buddy_index, memorder);

        if (left_child_free && requested_dir == mm_buddy_right_child) {
            DEBUG_PANIC("Left child left but right child wanted");
        }

        /*
        if (!left_child_free) {
            //DEBUG("MARKING RIGHT SIDE");
        } else  {
            //DEBUG("MARKING LEFT SIDE");
        } 
        */

        dir = left_child_free ? mm_buddy_left_child : mm_buddy_right_child;

    } else {

        if (requested_dir == mm_buddy_either || requested_dir == mm_buddy_left_child) {
            dir = mm_buddy_left_child;
        } else {
            dir = mm_buddy_right_child;
        }
    }

    return dir;
}

ll_node_t * _split_buddy_node(mm_area_t * area, unsigned int memorder, ll_node_t * buddy_node, mm_buddy_dir_t dir)
{
    ASSERT_PANIC(memorder > 0, "Splitting node at memorder 0");

    unsigned int child_index;
    ll_node_t * buddy_child;
    mm_buddy_dir_t marked_dir;

    unsigned int buddy_index = MM_BUDDY_INDEX(buddy_node);
    unsigned int buddy_status = _mark_buddy(area, buddy_index);

    marked_dir = _buddy_split_dir(area, buddy_node, buddy_status, dir);

    buddy_index = ((marked_dir == mm_buddy_left_child) ? 
                    MM_BUDDY_LEFT_CHILD(buddy_index) : MM_BUDDY_RIGHT_CHILD(buddy_index));


    // If this buddy was split and is now full, remove it from the free list
    if (buddy_status) {
        _remove_buddy_list(area, memorder, buddy_node);
    }

    // Add the buddy child to the memorder below the one we are splitting from
    buddy_child = _add_buddy_list(area, memorder - 1, buddy_index);
    ASSERT_PANIC(buddy_child, "Buddy child could not be created");

    return buddy_child;
}

ll_node_t * _find_free_buddy_node(mm_area_t * area, unsigned int memorder)
{
    ll_node_t * buddy_node;
    ll_node_t * new_node;
    unsigned int free_memorder;
    unsigned int free_index;

    ASSERT_PANIC(area && memorder <= MM_MAX_ORDER, "Area null or memorder outside range");

    // Go up the memorder list to find a free memorder
    for (free_memorder = memorder; free_memorder < MM_MAX_ORDER + 1; free_memorder++) {
        buddy_node = MM_AREA_FREE_BUDDY(area, free_memorder);
        if (buddy_node)
            break;
    }

    ASSERT_PANIC(buddy_node, "No free buddy node found");

    // We already found a free node
    if (buddy_node && free_memorder == memorder) {
        return buddy_node;
    }

    new_node = buddy_node;
    // Go from the free node found splitting until we get to desired memorder
    for (int i = (int)free_memorder; i > memorder; i--) {
        new_node = _split_buddy_node(area, i, new_node, mm_buddy_either);
    }

    return new_node;
}

ll_node_t * _split_to_target_addr(mm_area_t * area, ll_node_t * start_buddy_node, uint64_t target_addr, unsigned int target_memorder)
{
    uint64_t buddy_addr;
    mm_buddy_dir_t dir;
    ll_node_t * buddy_node = start_buddy_node;
    unsigned int buddy_index = MM_BUDDY_INDEX(start_buddy_node);
    unsigned int start_memorder = _memorder_from_buddy_index(buddy_index);

    for (unsigned int i = start_memorder; i > target_memorder; i--) {
        buddy_index = MM_BUDDY_INDEX(buddy_node);

        buddy_addr = MM_BUDDY_ADDR(area, buddy_index, i);
        unsigned int page_offset = MM_BITMAP_MEMORDER_OFFSET(buddy_index, i);

        dir = _buddy_split_dir_from_addr(buddy_addr, target_addr, i);

        buddy_node = _split_buddy_node(area, i, buddy_node, dir);
    }

    return buddy_node;
}

uint64_t _assign_buddy_node(mm_area_t * area, ll_node_t * buddy_node, unsigned int memorder, mm_buddy_dir_t dir)
{
    unsigned int page_index;
    unsigned int child_index;
    unsigned int buddy_status;
    unsigned int buddy_index = MM_BUDDY_INDEX(buddy_node);
    unsigned int memorder_pages = mm_memorder_to_pages(memorder);
    mm_page_t * pages = mm_global_area()->global_pages;
    
    ASSERT_PANIC(buddy_index <= MM_BITMAP_MAX_INDEX, "Buddy index exceeds max index");
    buddy_status = _mark_buddy(area, buddy_index);
    dir = _buddy_split_dir(area, buddy_node, buddy_status, dir);

    page_index = area->start_page_index + MM_BUDDY_PAGE_OFFSET(buddy_index, memorder);

    // If we explicitly want to mark a right child, add a memorder page offset 
    if (dir == mm_buddy_right_child) {
        //DEBUG("ADDING RIGHT CHILD OFFSET");
        page_index += memorder_pages;
    }

    //DEBUG_FUNC("Assign addr=", page_index * PAGE_SIZE);
    if (buddy_status) {
        _remove_buddy_list(area, memorder, buddy_node);
    }

    // Mark the pages as active at this buddy node
    for (int i = 0; i < memorder_pages; i++) {
        if (mm_page_is_valid(page_index + i)) {
            DEBUG_DATA("page_index=", (page_index + i) * PAGE_SIZE);
            DEBUG_PANIC("Page being marked is already valid");
        }

        mm_mark_page_valid(page_index + i);
    }
    
    return (uint64_t)(page_index * PAGE_SIZE);
}

int _coalesce_buddies(mm_area_t * area, ll_node_t * free_buddy, unsigned int start_memorder)
{   
    ll_node_t * node;
    ll_node_t * free_list;
    unsigned int buddy_sibling;
    unsigned int buddy_status;
    unsigned int buddy_index = MM_BUDDY_INDEX(free_buddy);
    bitmap_t * bitmap = area->bitmap;

    for (unsigned int i = start_memorder; i < MM_MAX_ORDER; i++)  {
        free_list = &area->free_buddy_list[i];
        buddy_status = _mark_buddy(area, buddy_index);

        // This buddy was split and is now going to be completly free
        if (buddy_status) {
            //DEBUG_DATA_DIGIT("buddy was split, now free. Further coalsece, removing=", buddy_index);
            node = ll_search_data(free_list, (void*)buddy_index);
            ASSERT_PANIC(node, "Split node not found in free list");
            _remove_buddy_list(area, i, node);
        } else {
            //DEBUG_DATA_DIGIT("Buddy was full, now split. adding index=", buddy_index);
            
            // This buddy node is now split, add back to free list
            _add_buddy_list(area, i, buddy_index);

            return 0;
        }
        // remove the sibling, the parent node will be added at the level we no long
        // need to coalesce

        buddy_index = MM_BUDDY_PARENT(buddy_index);
    }

    // We are coalescing up to the MM_MAX_ORDER buddy_node
    buddy_status = _mark_buddy(area, 0);


    return 0;
}

int mm_free_pages(uint64_t addr, unsigned int memorder)
{
    ll_node_t * buddy_node;
    ll_node_t * free_list;
    mm_buddy_dir_t dir;
    mm_area_t * area = mm_area_from_addr(addr);
    unsigned int buddy_index = MM_BITMAP_LOCAL_INDEX(addr, memorder);
    unsigned int buddy_status = bitmap_get(area->bitmap, buddy_index);
    unsigned int memorder_pages = MM_MEMORDER_TO_PAGES(memorder);
    unsigned int page_index = area->start_page_index + MM_BUDDY_PAGE_OFFSET(buddy_index, memorder);

    ASSERT_PANIC(area, "Area is invalid");
    ASSERT_PANIC(IS_ALIGNED(addr, memorder_pages * PAGE_SIZE), "mm_free_pages addr is not aligned to memorder");

    if (_buddy_split_dir_from_addr(MM_BUDDY_ADDR(area, buddy_index, memorder), addr, memorder) == mm_buddy_right_child) {
        page_index += memorder_pages;
    }

    for (int i = 0; i < memorder_pages; i++) {
        if (!mm_page_is_valid(page_index + i)) {
            DEBUG_PANIC("Page is not free that should be free");
        }

        mm_mark_page_free(page_index + i);
    }

    if (buddy_status) {
        //DEBUG_DATA_DIGIT("--Buddy is now free, coalescing. Index=", buddy_index);
        buddy_node = ll_search_data(&area->free_buddy_list[memorder], (void*)buddy_index);

        ASSERT_PANIC(buddy_node, "Buddy node not found even when split");

        _coalesce_buddies(area, buddy_node, memorder);
    } else  {
        // Buddy was full and is now free, add to free list
        //DEBUG_DATA_DIGIT("--Buddy is now split, other node is full. index=", buddy_index);
        _mark_buddy(area, buddy_index);
        _add_buddy_list(area, memorder, buddy_index);
    }

    area->free_page_num += memorder_pages;

    return 0;
}

int mm_reserve_pages(uint64_t addr, unsigned int memorder)
{
    ASSERT_PANIC(IS_ALIGNED(addr, mm_memorder_to_pages(memorder) * PAGE_SIZE), "resrve addr is not aligned");

    ll_node_t * buddy_node;
    mm_buddy_dir_t dir;
    uint64_t buddy_addr;
    unsigned int found_memorder;
    unsigned int page_index = MM_MEMORDER_INDEX(addr, 0);
    unsigned int buddy_index = MM_BITMAP_LOCAL_INDEX(addr, memorder);
    mm_area_t * area = MM_AREA_FROM_ADDR(addr);

    if (!mm_pages_are_free(addr, memorder)) {
        DEBUG_PANIC("Page is already reserved");
    }

    // Find a free buddy node in the range we want to reserve
    buddy_node = _find_free_ancestor_node(area, buddy_index, memorder);
    if (!buddy_node) {
        DEBUG_PANIC("Could not find free ancestor node");
    }

    // Split and mark buddy nodes to the target page range we want to reserve
    buddy_node = _split_to_target_addr(area, buddy_node, addr, memorder);
    if (!buddy_node) {
        DEBUG_PANIC("Could not split to a target addr");
    }

    buddy_addr = MM_BUDDY_ADDR(area, MM_BUDDY_INDEX(buddy_node), memorder);
    dir = _buddy_split_dir_from_addr(buddy_addr, addr, memorder);

    buddy_addr = _assign_buddy_node(area, buddy_node, memorder, dir);

    ASSERT_PANIC(buddy_addr == addr, "buddy addr does not equal assigned addr");

    area->free_page_num -= MM_MEMORDER_TO_PAGES(memorder);

    return 0;
}   

/* Returns the page_addr of the first page in the requested memorder range. */
uint64_t mm_alloc_pages(unsigned int memorder)
{
    mm_area_t * area;
    ll_node_t * node;
    unsigned int found_memorder;
    uint64_t found_addr;

    area = _find_free_area(memorder);

    node = _find_free_buddy_node(area, memorder);
    if (!node) {
        DEBUG_PANIC("Cannot find free buddy node");
        return 0;
    }

    found_memorder = _memorder_from_buddy_index(MM_BUDDY_INDEX(node));

    ASSERT_PANIC(found_memorder == memorder, "Found memorder is not the memorder we wanted");

    found_addr = _assign_buddy_node(area, node, memorder, mm_buddy_either);

    area->free_page_num -= MM_MEMORDER_TO_PAGES(memorder);

    return found_addr;
}

int mm_area_init(mm_global_area_t * global_area, mm_area_t * area, unsigned int start_page_index)
{
    ll_node_t * node;

    ASSERT_PANIC(global_area && area, "MM area init nulls found");
    
    //DEBUG_FUNC("Init area at addr=", area);
    //DEBUG_FUNC_DIGIT("-Start page index=", start_page_index);
    //DEBUG_FUNC_DIGIT("-Area bitmap entry start=", MM_BITMAP_ENTRY_START(area->phys_addr_start));
    memset(area, 0, sizeof(mm_area_t));

    area->free_page_num = MM_AREA_SIZE / PAGE_SIZE;
    area->start_page_index = start_page_index;
    area->phys_addr_start = start_page_index * PAGE_SIZE;
    area->bitmap = &global_area->global_bitmap[MM_BITMAP_ENTRY_START(area->phys_addr_start)];

    for (int i = 0; i < MM_MAX_ORDER + 1; i++) {
        /* Init the list to keep track of free buddys in this area. */
        ll_root_init(&area->free_buddy_list[i]);
        ll_node_init(&area->global_area_nodes[i], (void*)area);
    }

    // Add the first free buddy which is the max order, will also be added to the global area free list
    if(!_add_buddy_list(area, MM_MAX_ORDER, 0)) {
        DEBUG_PANIC("Could not add buddy list from area init");
        return 1;
    }

    return 0;
}

int mm_init(size_t mem_size, uint64_t *mem_start_addr)
{ 
    unsigned int num_pages = mem_size / PAGE_SIZE;
    size_t global_pages_size_pages_num = (num_pages * sizeof(mm_page_t)) / PAGE_SIZE;
    unsigned int area_num = mem_size / MM_AREA_SIZE;
    unsigned int global_bitmap_num = MM_BITMAPS_PER_AREA * (mem_size / MM_AREA_SIZE);
    size_t global_bitmap_size = global_bitmap_num * sizeof(bitmap_t);
    int ret = 0;

    DEBUG("---MM INIT START--");

    DEBUG_FUNC_DIGIT("-Global area page num=", num_pages);
    DEBUG_FUNC_DIGIT("-Global area pages struct size pages=", global_pages_size_pages_num);
    DEBUG_FUNC_DIGIT("-Global area sub area num=", area_num);
    DEBUG_FUNC_DIGIT("-Global area bitmap num=", global_bitmap_num);
    DEBUG_FUNC_DIGIT("-Global bitmap size=", global_bitmap_size);

    /* Init the node cache we will use for all the data structs. */
    ret = kalloc_cache_init(&mm_node_cache, sizeof(ll_node_t), 1, NULL, NULL, 
                            KALLOC_CACHE_NO_EXPAND_F | KALLOC_CACHE_NO_SHRINK_F | KALLOC_CACHE_NO_LINK_F);
    if (ret) {
        ASSERT(0);
        DEBUG("MM node cache init failed");
        return 1;
    }

    for (unsigned int i = 0; i < MM_INIT_NODE_CACHE_PAGES; i++) {
        void * page_mem = _early_page_data_alloc(1);
        kalloc_slab_t * slab = kalloc_cache_add_slab_pages(&mm_node_cache, page_mem, 1);
        if (!slab) {
            ASSERT(0);
            DEBUG("MM node cache slab init failed");
            return 1;
        }
    }

      /* Init the global area struct. */
    memset(&global_area, 0, sizeof(mm_global_area_t));
    spinlock_init(&global_area.lock);
    
    for (int i = 0; i < MM_MAX_ORDER + 1; i++) {
        ll_root_init(&global_area.free_areas_list[i]);
    }

    /* Init all the global pages over the mem space. */
    global_area.global_pages = _early_page_data_alloc(global_pages_size_pages_num);
    global_area.page_count = num_pages;

    /* Init the global bitmap over the mem space. */
    _align_early_mem(PAGE_SIZE);
    global_area.global_bitmap = _early_data_alloc(global_bitmap_size);
    
    /* Init the areas over the memory space. */
    _align_early_mem(PAGE_SIZE);
    global_area.global_areas = _early_data_alloc(sizeof(mm_area_t) * area_num);

    for (int i = 0; i < area_num; i++) {
        ret = mm_area_init(&global_area, &global_area.global_areas[i], (i * MM_AREA_SIZE)/PAGE_SIZE);
        if (ret) {
            ASSERT(0);
            DEBUG("MM area init failed");
            return 1;
        }
    }

    global_area.area_count = area_num;

    mm_initialized = 1;

    DEBUG("---MM INIT DONE--");
    return 0;
}
