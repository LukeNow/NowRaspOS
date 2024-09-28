#include <stddef.h>
#include <stdint.h>
#include <common/common.h>
#include <common/math.h>
#include <common/assert.h>
#include <kernel/mm.h>
#include <kernel/kalloc_page.h>
#include <kernel/kalloc_cache.h>

#define LEFT_BUDDY 1
#define RIGHT_BUDDY 2

/* Add a buddy_node to the area->free_list as well as checking if the area should be added to
 * the global_area->free_list. */
static void add_buddy_list(mm_area_t * area, kalloc_buddy_t * buddy)
{
    unsigned int memorder = buddy->buddy_memorder;
    ll_node_t * global_areas_list = &mm_global_area()->free_areas_list[memorder];
    ll_node_t * buddy_list = &area->free_buddy_list[memorder];

    // If the area list was empty before, we push this area to the global free lists
    if (!MM_AREA_FREE_BUDDY(area, memorder)) {
        if (ll_push_list(global_areas_list, &area->global_area_nodes[memorder]))  {
            DEBUG_PANIC("Could not push global areas list");
            return;
        }
    }

    if (ll_push_list(buddy_list, &buddy->buddy_node))  {
        DEBUG_PANIC("LL node push list failed");
        return;
    }
}

/* Remove from an area's->free_list and check if we need to remove an area from the 
 * global_areas->free_list. */
static void remove_buddy_list(mm_area_t * area, kalloc_buddy_t * buddy)
{   
    unsigned int memorder = buddy->buddy_memorder;
    ll_node_t * global_areas_list = &mm_global_area()->free_areas_list[memorder];
    ll_node_t * buddy_list = &area->free_buddy_list[memorder];

    if (buddy->buddy_node.next == &buddy->buddy_node || buddy->buddy_node.last == &buddy->buddy_node) {
        DEBUG_PANIC("Buddy node invlaid.");
    }

    if (ll_delete_node(buddy_list, &buddy->buddy_node))  {
        DEBUG_PANIC("Ll node delete fail");
        return;
    }

    // Reset the buddy node to indicate this node is not free
    buddy->buddy_node.next = NULL;
    buddy->buddy_node.last = NULL;

    // If there are no more free buddys of this memorder in this area, remove it from the global areas free list
    if (!MM_AREA_FREE_BUDDY(area, memorder))  {
        if (ll_delete_node(global_areas_list, &area->global_area_nodes[memorder])) {
            DEBUG_PANIC("LL area list delete fail");
            return;
        }
    }
}

void kalloc_init_buddy(mm_area_t * area, kalloc_buddy_t * buddy, unsigned int memorder, unsigned int add_to_list)
{
    memset(&buddy->buddy_node, 0, sizeof(ll_node_t));
    buddy->buddy_memorder = memorder;

    if (add_to_list)
        add_buddy_list(area, buddy);
}

unsigned int kalloc_get_buddy_page_index(kalloc_buddy_t * buddy)
{
    return (((uint64_t)buddy - ((uint64_t)mm_global_area()->global_buddies)) / sizeof(kalloc_buddy_t)) * 2;
}

kalloc_buddy_t * kalloc_get_buddy_from_page_index(unsigned int page_index)
{
    return &mm_global_area()->global_buddies[page_index / 2];
}

void kalloc_set_buddy_memorder(kalloc_buddy_t * buddy, unsigned int memorder)
{
    buddy->buddy_memorder = memorder;
}

kalloc_buddy_t * kalloc_get_buddy_from_addr(uint64_t addr)
{
    return kalloc_get_buddy_from_page_index(addr / PAGE_SIZE);
}

kalloc_buddy_t * kalloc_get_buddy_sibling(kalloc_buddy_t * buddy)
{
    ASSERT_PANIC(buddy->buddy_memorder <= MM_MAX_ORDER, "Getting buddy sibling for max order buddy. ");

    unsigned int memorder_pages = MM_MEMORDER_TO_PAGES(buddy->buddy_memorder);
    unsigned int index = kalloc_get_buddy_page_index(buddy);
    return kalloc_get_buddy_from_page_index(BITS_INVERT(index, memorder_pages));
}

static inline uint64_t get_buddy_addr(kalloc_buddy_t * buddy)
{
    return (kalloc_get_buddy_page_index(buddy)) * PAGE_SIZE;
}

static inline kalloc_buddy_t * get_buddy_from_node(ll_node_t * node)
{
    return STRUCT_P(node, kalloc_buddy_t, buddy_node);
}

static kalloc_buddy_t * get_buddy_parent(kalloc_buddy_t * buddy)
{
    kalloc_buddy_t * sibling = kalloc_get_buddy_sibling(buddy);
    unsigned int page_index = kalloc_get_buddy_page_index(buddy);
    unsigned int sibling_index = kalloc_get_buddy_page_index(sibling);

    // The parent buddy will be the buddy that has the leftmost index
    if (page_index > sibling_index)
        return sibling;

    return buddy;
}

static kalloc_buddy_t * get_buddy_child(kalloc_buddy_t * buddy, unsigned int child)
{   
    unsigned int index = kalloc_get_buddy_page_index(buddy);

    ASSERT_PANIC(buddy->buddy_memorder != 0, "Getting child of memorder 0 buddy.");

    if (child == RIGHT_BUDDY) {
        return kalloc_get_buddy_from_page_index(index + MM_MEMORDER_TO_PAGES(buddy->buddy_memorder - 1));
    }

    // The buddy of the left child is the current buddy, that would be later relabeled as the child
    return buddy;
}

//TODO make this more efficent instead of checking all pages, just check start pages or if nodes are on the free list
static unsigned int is_buddy_free(kalloc_buddy_t * buddy)
{   
    if (!buddy->buddy_node.next || !buddy->buddy_node.last)
        return 0;

    return mm_pages_are_free(kalloc_get_buddy_page_index(buddy), MM_MEMORDER_TO_PAGES(buddy->buddy_memorder));
}

/* Get the highest order free ancestor including the given buddy. */
static kalloc_buddy_t * get_free_ancestor(kalloc_buddy_t * buddy)
{
    kalloc_buddy_t * parent;
    unsigned int memorder = buddy->buddy_memorder;

    // This is the topmost buddy
    if (memorder == MM_MAX_ORDER)
        return buddy;

    while (memorder != MM_MAX_ORDER) {
        if (is_buddy_free(buddy))
            return buddy;

        parent = get_buddy_parent(buddy);
        if (parent == buddy)
            return buddy;

        buddy = parent;
        memorder = buddy->buddy_memorder;
    }

    return NULL;
}

static kalloc_buddy_t * next_child_from_addr(mm_area_t * area, kalloc_buddy_t * buddy, uint64_t target_addr)
{
    uint64_t buddy_addr = get_buddy_addr(buddy);
    unsigned int pages = MM_MEMORDER_TO_PAGES(buddy->buddy_memorder);

    ASSERT_PANIC(VAL_IN_RANGE(target_addr, buddy_addr, (pages * PAGE_SIZE)), "Buddy addr not in found range");
    
    if (VAL_IN_RANGE(target_addr, buddy_addr, (pages * PAGE_SIZE) / 2)) {
        return get_buddy_child(buddy, LEFT_BUDDY);
    }

    return get_buddy_child(buddy, RIGHT_BUDDY);
}

static kalloc_buddy_t * split_buddy(mm_area_t * area, kalloc_buddy_t * buddy)
{
    unsigned int memorder = buddy->buddy_memorder;
    kalloc_buddy_t * left = get_buddy_child(buddy, LEFT_BUDDY);
    kalloc_buddy_t * right = get_buddy_child(buddy, RIGHT_BUDDY);

    if (memorder == 0) {
        DEBUG_PANIC("Splitting buddy of memorder 0");
        return NULL;
    }

    remove_buddy_list(area, buddy);

    kalloc_set_buddy_memorder(left, memorder - 1);
    add_buddy_list(area, left);

    // We are splitting to a memorder 0 buddy, which is the same buddy but for single pages
    if (memorder == 1)
        return left;

    kalloc_set_buddy_memorder(right, memorder - 1);
    add_buddy_list(area, right);

    return left;
}

static kalloc_buddy_t * split_to_target_addr(mm_area_t * area, kalloc_buddy_t * start_buddy, uint64_t target_addr, unsigned int target_memorder)
{
    kalloc_buddy_t * curr_buddy = start_buddy;
    kalloc_buddy_t * next_buddy;
    uint64_t buddy_addr;

    ASSERT_PANIC(start_buddy->buddy_memorder >= target_memorder, "Start buddy below target memorder. ");

    if (VAL_IN_RANGE_INCLUSIVE(target_addr, get_buddy_addr(start_buddy), MM_MEMORDER_TO_PAGES(start_buddy->buddy_memorder) * PAGE_SIZE) && 
        start_buddy->buddy_memorder == target_memorder) {
        return start_buddy;
    } else if (start_buddy->buddy_memorder == target_memorder) {
        DEBUG_PANIC("Already at target memorder but not at correct buddy address. ");
    }
   
    for (int i = (int)start_buddy->buddy_memorder; i > (int)target_memorder; i--) {
        buddy_addr = get_buddy_addr(curr_buddy);
        next_buddy = next_child_from_addr(area, curr_buddy, target_addr);
        split_buddy(area, curr_buddy);
        curr_buddy = next_buddy;
    }

    ASSERT_PANIC(VAL_IN_RANGE_INCLUSIVE(target_addr, get_buddy_addr(curr_buddy), MM_MEMORDER_TO_PAGES(curr_buddy->buddy_memorder) * PAGE_SIZE), "Split to buddy but not to correct buddy. ");

    return curr_buddy;
}

static kalloc_buddy_t * split_to_target_memorder(mm_area_t * area, kalloc_buddy_t * start_buddy, unsigned int target_memorder)
{
    kalloc_buddy_t * curr_buddy = start_buddy;
    kalloc_buddy_t * next_buddy;

    ASSERT_PANIC(start_buddy->buddy_memorder >= target_memorder, "Start buddy below target memorder.");

    for (int i = (int)start_buddy->buddy_memorder; i > (int)target_memorder; i--) {
        // Default to splitting from left child
        next_buddy = get_buddy_child(curr_buddy, LEFT_BUDDY);
        split_buddy(area, curr_buddy);
        curr_buddy = next_buddy;
    }

    return curr_buddy;
}

static kalloc_buddy_t * find_free_buddy(mm_area_t * area, unsigned int memorder)
{
    ll_node_t * buddy_node;
    kalloc_buddy_t * buddy;
    unsigned int free_memorder;

    ASSERT_PANIC(area && memorder <= MM_MAX_ORDER, "Area null or memorder outside range");

    // Go up the memorder list to find a free memorder
    for (free_memorder = memorder; free_memorder < MM_MAX_ORDER + 1; free_memorder++) {
        buddy_node = MM_AREA_FREE_BUDDY(area, free_memorder);
        if (buddy_node)
            break;
    }

    ASSERT_PANIC(buddy_node, "No free buddy node found");

    buddy = get_buddy_from_node(buddy_node);

    // We already found a free node
    if (buddy_node && buddy->buddy_memorder == memorder)
        return buddy;

    return split_to_target_memorder(area, buddy, memorder);
}

static unsigned int get_buddy_free_page(kalloc_buddy_t * buddy)
{
    unsigned int page_index = kalloc_get_buddy_page_index(buddy);
    ASSERT_PANIC(buddy->buddy_memorder == 0, "Assign page Buddy is not memorder 0.");

    if (!mm_page_is_valid(page_index))
        return page_index;

    return page_index + 1;
}

static uint64_t assign_buddy_page(mm_area_t * area, kalloc_buddy_t * buddy, unsigned int assign_page_index)
{
    ASSERT_PANIC(kalloc_get_buddy_from_page_index(assign_page_index) == buddy, "assigning page index does not map to buddy.");
    ASSERT_PANIC(buddy->buddy_memorder == 0, "Assign page Buddy is not memorder 0.");

    if (mm_page_is_valid(assign_page_index)) {
        DEBUG_PANIC("Assigning page when page is valid. ");
        return 0;
    }

    // If the other page of this buddy is already reserved, remove this buddy from free list since it is full now
    if (mm_page_is_valid(assign_page_index ^ 1)) {
        remove_buddy_list(area, buddy);
    }

    mm_mark_page_valid(assign_page_index);

    return (uint64_t)(assign_page_index * PAGE_SIZE);
}

static void free_buddy_page(mm_area_t * area, kalloc_buddy_t * buddy, unsigned int free_page_index)
{
    ASSERT_PANIC(kalloc_get_buddy_from_page_index(free_page_index) == buddy, "assigning page index does not map to buddy.");
    ASSERT_PANIC(buddy->buddy_memorder == 0, "Assign page Buddy is not memorder 0.");

    if (!mm_page_is_valid(free_page_index)) {
        DEBUG_PANIC("Assinging page when page is valid. ");
        return;
    }

    // If the other memorder 0 page is free, add this buddy to the free list
    if (mm_page_is_valid(free_page_index ^ 1)) {
        add_buddy_list(area, buddy);
    } else { // This buddy is now completly free, set it as a memorder 1 buddy and add back to free list for coalescing
        remove_buddy_list(area, buddy);
        kalloc_set_buddy_memorder(buddy, 1);
        add_buddy_list(area, buddy);
    }

    mm_mark_pages_free(free_page_index, 1);
}

static uint64_t assign_buddy(mm_area_t * area, kalloc_buddy_t * buddy)
{   
    kalloc_buddy_t * sibling = kalloc_get_buddy_sibling(buddy);
    unsigned int memorder_pages = MM_MEMORDER_TO_PAGES(buddy->buddy_memorder);
    unsigned int page_index = kalloc_get_buddy_page_index(buddy);

    if (!is_buddy_free(buddy)) {
        DEBUG_PANIC("Non zero memorder assign is not complely free.");
    }

    mm_mark_pages_valid(page_index, memorder_pages);
    remove_buddy_list(area, buddy);

    return (uint64_t)(page_index * PAGE_SIZE);
}

static void coalesce_buddies(mm_area_t * area, kalloc_buddy_t * buddy)
{   
    kalloc_buddy_t * sibling;
    kalloc_buddy_t * parent;

    for (unsigned int i = buddy->buddy_memorder; i < MM_MAX_ORDER; i++)  {
        sibling = kalloc_get_buddy_sibling(buddy);

        if (!buddy->buddy_memorder || sibling->buddy_memorder != buddy->buddy_memorder || !is_buddy_free(buddy) || !is_buddy_free(sibling))
            break;

        parent = get_buddy_parent(buddy);

        remove_buddy_list(area, buddy);
        remove_buddy_list(area, sibling);

        kalloc_set_buddy_memorder(parent, i + 1);
        add_buddy_list(area, parent);

        buddy = parent;
    }
}

int kalloc_page_free_pages(uint64_t addr, flags_t flags)
{
    mm_area_t * area = mm_area_from_addr(addr);
    unsigned int page_index = addr / PAGE_SIZE;
    kalloc_buddy_t * buddy = kalloc_get_buddy_from_page_index(page_index);
    unsigned int memorder_pages =  MM_MEMORDER_TO_PAGES(buddy->buddy_memorder);

    ASSERT_PANIC(area, "Area from addr not found. ");
    ASSERT_PANIC(mm_is_initialized(), "Mm is not initialized.");
    ASSERT_PANIC(IS_ALIGNED(addr, memorder_pages * PAGE_SIZE), "mm_free_pages addr is not aligned to memorder");

    buddy = kalloc_get_buddy_from_page_index(page_index);

    if (buddy->buddy_memorder == 0) {
        free_buddy_page(area, buddy, page_index);
    } else {
        mm_mark_pages_free(page_index, memorder_pages);
        add_buddy_list(area, buddy);
    }

    coalesce_buddies(area, buddy);

    area->free_page_num += memorder_pages;

    return 0;
}

int kalloc_page_reserve_pages(uint64_t addr, unsigned int memorder, flags_t flags)
{
    kalloc_buddy_t * buddy;
    uint64_t buddy_addr;
    unsigned int page_index = addr / PAGE_SIZE;
    mm_area_t * area = mm_area_from_addr(addr);

    ASSERT_PANIC(area, "Free area not found from addr");
    ASSERT_PANIC(mm_is_initialized(), "Mm is not initialized.");
    ASSERT_PANIC(IS_ALIGNED(addr, MM_MEMORDER_TO_PAGES(memorder) * PAGE_SIZE), "resrve addr is not aligned");

    if (!mm_pages_are_free(page_index, MM_MEMORDER_TO_PAGES(memorder))) {
        DEBUG_PANIC("Page is already reserved");
    }

    buddy = kalloc_get_buddy_from_page_index(page_index);
    // Make sure we get the topmost free ancestor so we can preserve the buddy structure
    // and split it later to the correct buddy we want to reserve
    buddy = get_free_ancestor(buddy);

    ASSERT_PANIC(buddy, "No free buddy found at given addr");
   
    buddy = split_to_target_addr(area, buddy, addr, memorder);
    
    ASSERT_PANIC(buddy->buddy_memorder == memorder, "Found memorder is not the memorder we wanted.");

    if (memorder == 0) {
        buddy_addr = assign_buddy_page(area, buddy, page_index);
    } else {
        buddy_addr = assign_buddy(area, buddy);
    }

    ASSERT_PANIC(buddy_addr == addr, "buddy addr does not equal assigned addr");

    area->free_page_num -= MM_MEMORDER_TO_PAGES(memorder);

    return 0;
}   

/* Returns the page_addr of the first page in the requested memorder range. */
uint64_t kalloc_page_alloc_pages(unsigned int memorder, flags_t flags)
{
    mm_area_t * area;
    unsigned int found_memorder;
    uint64_t buddy_addr;
    kalloc_buddy_t * buddy;

    ASSERT_PANIC(mm_is_initialized(), "Mm is not initialized.");

    area = mm_find_free_area(memorder);
    if (!area) {
        DEBUG_PANIC("No free area found, full?");
    }

    buddy = find_free_buddy(area, memorder);
    if (!buddy) {
        DEBUG_PANIC("Cannot find free buddy node");
        return 0;
    }

    found_memorder = buddy->buddy_memorder;
    ASSERT_PANIC(found_memorder == memorder, "Found memorder is not the memorder we wanted");

    if (memorder == 0) {
        buddy_addr = assign_buddy_page(area, buddy, get_buddy_free_page(buddy));
    } else {
        buddy_addr = assign_buddy(area, buddy);
    }

    area->free_page_num -= MM_MEMORDER_TO_PAGES(memorder);

    return buddy_addr;
}
