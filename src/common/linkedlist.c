#include <stddef.h>
#include <stdint.h>
#include <common/linkedlist.h>
#include <common/assert.h>
#include <kernel/uart.h>
#include <kernel/printf.h>
#include <common/string.h>
#include <common/lock.h>

#define HAS_LAST(HEAD) ((HEAD)->type & (DLL_NODE_T))
#define HAS_DATA(HEAD) ((HEAD)->type & (SLL_NODE_T | DLL_NODE_T))

#define NULL_NODE_MSG "Node is Null."
#define DATA_UNSUPPORTED_MSG "Data is not supported with this type."
#define CHECK_NULL(NODES) ASSERT_PANIC((NODES), NULL_NODE_MSG);
#define CHECK_HAS_DATA(HEAD) ASSERT_PANIC(HAS_DATA(HEAD), DATA_UNSUPPORTED_MSG)

size_t ll_type_size(ll_type_t type)
{
    switch(type) {
        case LIST_NODE_T: return sizeof(list_node_t);
        case SLL_NODE_T: return sizeof(sll_node_t);
        case DLL_NODE_T: return sizeof(dll_node_t);
    }

    DEBUG_PANIC("Non valid LL type");

    return 0;
}

int ll_head_init(ll_head_t * head, ll_type_t type)
{
    CHECK_NULL(head);
    
    memset(head, 0, sizeof(ll_head_t));

    lock_init(&head->lock);
    head->type = type;

    return 0;
}

int ll_node_init(ll_node_t * node, void * data, ll_type_t type)
{
    CHECK_NULL(node);

    memset(node, 0, ll_type_size(type));

    switch(type) {
        case DLL_NODE_T:
        case SLL_NODE_T:
            node->sll.data = data;
    }

    return 0;
}

/* Append NODE after LAST.
 * If last is NULL then append to the front of the list. */
static int _ll_append_node(ll_head_t * head, ll_node_t * node, ll_node_t * last)
{   
    ll_node_t * next;
    
    CHECK_NULL(head && node);

    // Empty list add
    if (!head->next) {
        ASSERT_PANIC(!last, "Last should be Null if this is the only one in the list");
        ASSERT_PANIC(!head->count,"Head count should be 0");

        head->next = node;
        head->last = node;

        return 0;
    }

    // Front of list add
    if (!last || node == last) {
        node->list.next = head->next;
        next = head->next;
        head->next = node;
    } else {
        next = last->list.next;
        last->list.next = node;
        node->list.next = next;
    }

    if (HAS_LAST(head)) {
        node->dll.last = last; // can be NULL
        if (next)
            next->dll.last = node;
    }

    // End of list add
    if (head->last == last) {
        head->last = node;
    }

    return 0;
}

/* Delete NODE that is after LAST.
 * If last is NULL then delete the back of the list. */
static int _ll_delete_node(ll_head_t * head, ll_node_t * node, ll_node_t * last)
{
    ll_node_t * next;

    CHECK_NULL(node);

    // Only one element in list
    if (head->count == 1) {
        head->next = NULL;
        head->last = NULL;

        return 0;
    }

    next = node->list.next;
    // Front of list remove
    if (!last) {
        head->next = next;
    } else {
        last->list.next = next;
    }

    if (HAS_LAST(head)) {
        if (next)
            next->dll.last = node->dll.last;
    }

    if (head->last == node) {
        head->last = last;
    }

    return 0;
}

int ll_node_exists(ll_head_t * head, ll_node_t * node)
{
    ll_node_t * p;

    CHECK_NULL(head && node);
   
    if (head == node) {
        DEBUG_PANIC("Checking if root exists.");
    }

    if (head->count == 0)
        return 0;

    LL_ITER_LIST(head, p) {
        if (p == node)
            return 1;
    }

    return 0;
}

/* Append NODE after LAST.
 * If last is NULL then append to the front of the list. */
int ll_insert_node(ll_head_t * head, ll_node_t * node, ll_node_t * last)
{
    ll_node_t * p;
    int ret = 0;

    CHECK_NULL(head && node);
    
    if (ll_node_exists(head, node)) { // TOGGLE THIS AS DEBUG
        DEBUG_THROW("Node does not exist");
        return 1;
    }

    if (ret = _ll_append_node(head, node, last)) {
        return ret;
    }

    head->count += 1;

    return ret;
}

int ll_delete_node(ll_head_t * head, ll_node_t * node)
{
    ll_node_t * p;
    ll_node_t * found_last = NULL;
    int ret = 0;

    CHECK_NULL(head && node);

    if (head->count == 0)
        return 1;

    if (!ll_node_exists(head, node)) { // TOGGLE THIS AS DEBUG
        DEBUG_PANIC("The node does not exist."); // Again throw an assert here to catch if we wanted to delete something that isnt in the list
        return 1;
    }

    // We need to find the last node before the one we want to delete
    if (!HAS_LAST(head)) {
        LL_ITER_LIST(head, p) {
            if (p == node)
                break;
            found_last = p;
        }

    } else {
        found_last = node->dll.last;
    }

    //DEBUG_DATA("Found last=", found_last);
    if (ret = _ll_delete_node(head, node, found_last)) {
        return ret;
    }

    head->count -= 1;

    return ret;
}

ll_node_t * ll_pop_list(ll_head_t * head)
{
    ll_node_t * node;

    CHECK_NULL(head);

    if (!head->count)
        return NULL;

    node = head->last;
    if (ll_delete_node(head, node)) {
        return NULL;
    }

    return node;
}

int ll_push_list(ll_head_t * head, ll_node_t * node)
{
    int ret = 0;

    CHECK_NULL(head && node);

    if (ret = ll_node_exists(head, node)) { // TOGGLE THIS AS DEBUG
        ASSERT(0); // Again throw an assert here to catch if we wanted to delete something that isnt in the list
        return ret;
    }

    if (ret = _ll_append_node(head, node, head->last)) {
        return ret;
    }

    head->count += 1;

    return ret;
}


ll_node_t * ll_peek_first(ll_head_t * head)
{   
    if (!head->count)
        return NULL;

    return head->next;
}

ll_node_t * ll_peek_last(ll_head_t * head)
{   
    if (!head->count)
        return NULL;

    return head->last;
}

/* Match the first occurence of a node with the data element. */
ll_node_t * ll_search_data(ll_head_t * head, void * data)
{   
    ll_node_t * p;

    CHECK_NULL(head);
    CHECK_HAS_DATA(head);

    LL_ITER_LIST(head, p) {
        if (p->sll.data == data)
            return p;
    }

    return NULL;
}

unsigned int ll_list_size(ll_head_t * head)
{
    return head->count;
}

unsigned int ll_is_empty(ll_head_t * head)
{   
    if (ll_list_size(head))
        return 0;

    return 1;
}

int ll_delete_list_data(ll_head_t * head, void * data)
{
    ll_node_t * p;

    CHECK_NULL(head);
    CHECK_HAS_DATA(head);
    
    LL_ITER_LIST(head, p) {
        if (p->sll.data == data) {
            return ll_delete_node(head, p);
        }
    }

    return 1;
}
