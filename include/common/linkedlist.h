#ifndef __LINKED_LIST_H
#define __LINKED_LIST_H
#include <stddef.h>
#include <stdint.h>

/* Circular doubly linked list.
 * For the root node data represents an unsigned int for the size of the list.
 */

typedef struct linked_list_node_t {
    void * next;
    void * last;
    void * data;
} ll_node_t;

#define LL_ROOT_ZERO(X) do {(X).next = &X; (X).last = &X; (X).data = NULL;} while(0)
#define LL_ROOT_INIT(X) static ll_node_t (X)
#define LL_ITERATE_LIST(root, p) for ((p) = (root)->next; (p) != (root); (p) = (p)->next)
#define LL_ITERATE_LIST_REV(root, p) for ((p) = (root)->last; (p) != (root); (p) = (p)->last)

int ll_node_exists(ll_node_t * root, ll_node_t * node);
int ll_delete_node(ll_node_t * root, ll_node_t * node);
int ll_insert_node(ll_node_t * root, ll_node_t * last, ll_node_t * node);
ll_node_t * ll_pop_list(ll_node_t * root);
int ll_push_list(ll_node_t * root, ll_node_t * node);
int ll_append_list_index(ll_node_t * root, ll_node_t * node, int index);
int ll_delete_list_index(ll_node_t * root, int index);
void ll_traverse_list(ll_node_t * root);
int ll_node_init(ll_node_t * node, void * data);
int ll_root_init(ll_node_t * root);

#endif
