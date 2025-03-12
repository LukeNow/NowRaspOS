#include <stdint.h>
#include <stddef.h>

#include <common/queue.h>

void enqueue_head(queue_t q, queue_entry_t qe)
{   
    qe->next = q->next;
    qe->prev = q;
    qe->next->prev = qe;
    q->next = qe;
}

void enqueue_tail(queue_t q, queue_entry_t qe)
{
    qe->next = q;
    qe->prev = q->prev;
    q->prev->next = qe;
    q->prev = qe;
}

queue_entry_t dequeue_head(queue_t q)
{
    queue_entry_t next = q->next;

    if (next == q)
        return (queue_entry_t)NULL;

    q->next = q->next->next;
    q->next->prev = q;
    return next;
}

queue_entry_t dequeue_tail(queue_t q)
{
    queue_entry_t prev = q->prev;

    if (prev == q)
        return (queue_entry_t)NULL;
    
    q->prev = q->prev->prev;
    q->prev->next = q;

    return prev;
}

void rmqueue(queue_entry_t qe)
{
    qe->next->prev = qe->prev;
    qe->prev->next = qe->next;
}
