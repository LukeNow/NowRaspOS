#ifndef __LINKED_LIST_H
#define __LINKED_LIST_H

#include <stddef.h>
#include <stdint.h>

// One hot encoding
typedef enum {
    LIST_NODE_T = 1,
    SLL_NODE_T = 2,
    DLL_NODE_T = 4
} ll_type_t;

/* Singly linked list with no data*/
typedef struct list_node_t {
    void * next;
} list_node_t;

/* Singly linked list with data. */
typedef struct sll_node_t {
    void * next;
    void * data;
} sll_node_t;

/* Circular doubly linked list with data. */
typedef struct dll_node_t {
    void * next;
    void * data;
    void * last;
} dll_node_t;

typedef union ll_node_t {
    list_node_t list;
    sll_node_t sll;
    dll_node_t dll;
} ll_node_t;

typedef struct ll_head_t {
    ll_node_t * next;
    ll_node_t * last;
    unsigned int count;
    ll_type_t type;
} ll_head_t __attribute__ ((aligned (8)));


#define LL_HEAD_ZERO(X, TYPE) do {(X).next = &X; (X).last = &X; (X).data = NULL; (X).lock = 0; (X).type = (TYPE)} while(0)
#define LL_HEAD_INIT(X) static ll_head_t (X)

#define LL_ITER_LIST(HEAD, P) for ((P) = (HEAD)->next; (P); (P) = (P)->list.next)
#define DLL_ITER_LIST(HEAD, P) for ((P) = (HEAD)->next; (P); (P) = (P)->next)
#define SLL_ITER_LIST(HEAD, P) for ((P) = (HEAD)->next; (P); (P) = (P)->next)


size_t ll_type_size(ll_type_t type);
int ll_node_exists(ll_head_t * head, ll_node_t * node);
int ll_delete_node(ll_head_t * head, ll_node_t * node);
ll_node_t * ll_pop_list(ll_head_t * head);
int ll_push_list(ll_head_t * head, ll_node_t * node);
int ll_insert_node(ll_head_t * head, ll_node_t * node, ll_node_t * last);
int ll_queue_list(ll_head_t * head, ll_node_t * node);
ll_node_t * ll_search_data(ll_head_t * head, void * data);
int ll_delete_list_data(ll_head_t * head, void * data);
ll_node_t * ll_peek_first(ll_head_t * head);
ll_node_t * ll_peek_last(ll_head_t * head);
unsigned int ll_list_size(ll_head_t * head);
int ll_node_init(ll_node_t * node, void * data, ll_type_t type);
int ll_head_init(ll_head_t * head, ll_type_t type);


#endif
