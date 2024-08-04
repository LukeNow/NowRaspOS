#include <stddef.h>
#include <stdint.h>
#include <common/linkedlist.h>
#include <common/assert.h>
#include <kernel/uart.h>
#include <kernel/printf.h>
#include <common/string.h>

int ll_root_init(ll_node_t * root)
{
     if (!root) {
        ASSERT(0);
        return 1;
    }

    memset(root, 0, sizeof(ll_node_t));

    root->last = root;
    root->next = root;

    return 0;
}

int ll_node_init(ll_node_t * node, void * data)
{
      if (!node) {
        ASSERT(0);
        return 1;
    }

    memset(node, 0, sizeof(ll_node_t));

    node->data = data;

    return 0;
}

/* Append the NEXT node after NODE */
static int _ll_append_node(ll_node_t * node, ll_node_t * next)
{   
    if (!node || !next) {
        ASSERT(0);
        return 1;
    }

    next->next = node->next;
    next->last = node;
    
    ((ll_node_t *)node->next)->last = next;
    node->next = next;

    return 0;
}

static int _ll_delete_node(ll_node_t * node, ll_node_t * unused __attribute__((unused)))
{
    ll_node_t * last;
    ll_node_t * next;

    if (!node) {
        ASSERT(0);
        return 1;
    }

    last = node->last;
    next = node->next;

    // This is the root node and only node in the list
    if (last == node || next == node) {
        ASSERT(0);
        return 1;
    }

    last->next = next;
    next->last = last;

    return 0;
}

// By convention 0 is the first node after root
// -1 is the last node
static int _ll_list_index_op(ll_node_t * root, ll_node_t * node, int index, int (* op)(ll_node_t *, ll_node_t *))
{
    ll_node_t * p;
    int count = 0;
    int ret = 0;

    if (!root) {
        ASSERT(0);
    }

    if (index < 0) {
        count = -1;
        LL_ITERATE_LIST_REV(root, p) {
            if (count == index) {
                ret = op(p, node);
                break;
            }

            count --;
            ret = 1;
        }
    }

    if (index >= 0) {
        LL_ITERATE_LIST(root, p) {
            if (count == index) {
                ret = op(p, node);
                break;
            }
            count ++;
            ret = 1;
        }     
    }

    return ret;
}

int ll_node_exists(ll_node_t * root, ll_node_t * node)
{
    ll_node_t * p;
    
    if (!root || !node) {
        ASSERT(0);
        return 1;
    }

    if (root == node) // Do we treat the root as a valid data node? Probably since its circular
        return 0;

    // Size is 0
    if ((unsigned int)root->data == 0)
        return 1;

    LL_ITERATE_LIST(root, p) {
        if (p == node)
            return 0;
    }
    
    return 1;
}

int ll_insert_node(ll_node_t * root, ll_node_t * last, ll_node_t * node)
{
    ll_node_t * p;
    int ret = 0;

    if (!root || !last || !node) {
        ASSERT(0);
        return 1;
    }

    if (ret = ll_node_exists(root, last)) { // TOGGLE THIS AS DEBUG
        ASSERT(0); // Throw an assert here so we catch stuff we didnt mean to look up
        return ret;
    }

    if (ret = _ll_append_node(last, node)) {
        return ret;
    }

    root->data = ((unsigned int)root->data) + 1;
    return ret;
}

int ll_delete_node(ll_node_t * root, ll_node_t * node)
{
    ll_node_t * next;
    int ret = 0;

    if (!root || root == node) {
        ASSERT(0);
        return 1;
    }

    // Size is 0
    if ((unsigned int)root->data == 0)
        return 1;

    if (ret = ll_node_exists(root, node)) { // TOGGLE THIS AS DEBUG
        ASSERT(0); // Again throw an assert here to catch if we wanted to delete something that isnt in the list
        return ret;
    }

    if (ret = _ll_delete_node(node, NULL)) {
        return ret;
    }

    root->data = ((unsigned int)root->data) - 1;
    return ret;
}

ll_node_t * ll_pop_list(ll_node_t * root)
{
    ll_node_t * ret;

    if (!root) {
        ASSERT(0);
        return NULL;
    }

    // Popping empty list
    if (root->next == root || (unsigned int)root->data == 0) {
        return NULL;
    }

    ret = root->last;
    if (_ll_delete_node(ret, NULL)) {
        return NULL;
    }

    root->data = ((unsigned int)root->data) - 1;
    ASSERT((uint64_t)ret);

    return ret;
}

int ll_push_list(ll_node_t * root, ll_node_t * node)
{
    int ret = 0;

    if (!root || !node) {
        ASSERT(0);
        return 1;
    }

    // This is valid even in the case of just the root
    if (ret = _ll_append_node(root->last, node)) {
        return ret;
    }

    root->data = ((unsigned int)root->data) + 1;

    return ret;
}

ll_node_t * ll_peek_first_list(ll_node_t * root)
{   
    // Root data is empty so the list is empty
    if (root->next == root)
        return NULL;

    return root->next;
}

ll_node_t * ll_peek_last_list(ll_node_t * root)
{   
    // Root data is empty so the list is empty
    if (!root->data)
        return NULL;

    return root->next;
}

/* Match the first occurence of a node with the data element. */
ll_node_t * ll_search_data(ll_node_t * root, void * data)
{   
    ll_node_t * node_p;
    LL_ITERATE_LIST(root, node_p) {
        if (node_p->data == data)
            return node_p;
    }

    return NULL;
}

unsigned int ll_list_size(ll_node_t * root)
{
    return (unsigned int )root->data;
}

unsigned int ll_is_empty(ll_node_t * root)
{   
    if (!root->data) {
        ASSERT_PANIC(root->next == root, "Root size is 0 but node does not point to self");
        return 1;
    }

    return 0;
}

int ll_delete_list_data(ll_node_t * root, void * data)
{
    ll_node_t * p;
    LL_ITERATE_LIST(root, p) {
        if (p->data == data) {
            return ll_delete_node(root, p);
        }
    }

    return 1;
}

ll_node_t * ll_search_list_index(ll_node_t * root, unsigned int index)
{   
    ll_node_t * p;
    unsigned int count = 0;

    LL_ITERATE_LIST(root, p) {
        if (count == index) {
            return p;
        }

        count++;
    }

    return NULL;
}

int ll_append_list_index(ll_node_t * root, ll_node_t * node, int index)
{   
    int ret = 0;

    ret = _ll_list_index_op(root, node, index, _ll_append_node);
    
    if (!ret) {
        root->data = ((unsigned int)root->data) + 1;
    }

    return ret;
}

int ll_delete_list_index(ll_node_t * root, int index)
{
    int ret = 0;
    
    if (!root) {
        ASSERT(0);
        return 1;
    }

    if ((unsigned int)root->data == 0) {
        return 1;
    }

    ret = _ll_list_index_op(root, NULL, index, _ll_delete_node);

    if (!ret) {
        root->data = ((unsigned int)root->data) - 1;
    }

    return ret;
}

void ll_traverse_list(ll_node_t * root)
{
    ll_node_t * p;



    printf("LL: Starting list traversal\n");
    printf("LL LIST SIZE ");
    uart_hex(((unsigned int)root->data));
    printf("\n");
    
    LL_ITERATE_LIST(root, p) {
        printf("LL DATA: ");
        uart_hex(((unsigned int)p->data));
        printf("\n");
    }
}