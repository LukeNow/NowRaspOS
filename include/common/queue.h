#ifndef __QUEUE_H
#define __QUEUE_H

#include <common/common.h>

struct queue_entry {
    struct queue_entry * next;
    struct queue_entry * prev;
};

typedef struct queue_entry queue_chain_t;
typedef struct queue_entry queue_head_t;
typedef struct queue_entry* queue_t;
typedef struct queue_entry* queue_entry_t;

#define queue_init(q)   ((q)->next = (q)->prev = q)
#define queue_zero(q)   ((q)->next = (q)->prev = NULL)
#define queue_valid(q)  ((q) && (q)->next && (q)->prev)
#define queue_first(q)  ((q)->next)
#define queue_last(q)   ((q)->last)
#define queue_prev(qe)  ((qe)->prev)
#define queue_next(qe)   ((qe)->next)
#define queue_end(q, qe)    ((q) == (qe))
#define queue_empty(q)  queue_end((q), queue_first(q))

void enqueue_head(queue_t q, queue_entry_t qe);
void enqueue_tail(queue_t q, queue_entry_t qe);
queue_entry_t dequeue_head(queue_t q);
queue_entry_t dequeue_tail(queue_t q);
void rmqueue(queue_entry_t qe);


#define qe_chain_access(qe, struct_name, qe_chain_name) (struct_name *)((qe) && queue_valid((qe)) ? (STRUCT_P((qe), struct_name, qe_chain_name)) : NULL)

#define queue_push_head(q, struct_p, qe_chain_name)     \
{                                                       \
    enqueue_head((q), &(struct_p)->qe_chain_name);      \
}

#define queue_push_tail(q, struct_p, qe_chain_name)     \
{                                                       \
    enqueue_tail((q), &(struct_p)->qe_chain_name);      \
}

#define queue_pop_tail(q, ret, struct_name, qe_chain_name)      \
{                                                               \
    queue_entry_t qe;                                           \
    qe = dequeue_tail((q));                                     \
    (ret) = qe_chain_access(qe, struct_name, qe_chain_name);    \
    queue_zero(qe);                                             \
}

#define queue_pop_head(q, ret, struct_name, qe_chain_name)      \
{                                                               \
    queue_entry_t qe;                                           \
    qe = dequeue_head((q));                                     \
    (ret) = qe_chain_access(qe, struct_name, qe_chain_name);    \
    queue_zero(qe);                                             \
}

#define queue_next_safe(qe, prev_qe) ((queue_next(prev_qe) == qe) ? queue_next(qe) : queue_next(prev_qe))

#define queue_iter_safe(q_head, qe, qe_prev) \            
    for ((qe) = queue_first(q_head), (qe_prev) = queue_prev(qe); queue_valid(qe) && !queue_end((q_head), (qe)); (qe) = queue_next_safe((qe), (qe_prev)), (qe_prev) = queue_prev(qe))

#define queue_iter(q_head, qe) \
    for ((qe) = queue_first(q_head); queue_valid(qe) && !queue_end((q_head), (qe)); (qe) = queue_next(qe))

#endif