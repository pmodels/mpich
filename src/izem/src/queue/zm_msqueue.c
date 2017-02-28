/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * See COPYRIGHT in top-level directory.
 */

#include "queue/zm_msqueue.h"
#include "mem/zm_hzdptr.h"

int zm_msqueue_init(zm_msqueue_t *q) {
    zm_msqnode_t* node = (zm_msqnode_t*) malloc(sizeof(zm_msqnode_t));
    node->data = NULL;
    node->next = ZM_NULL;
    zm_atomic_store(&q->head, (zm_ptr_t)node, zm_memord_release);
    zm_atomic_store(&q->tail, (zm_ptr_t)node, zm_memord_release);
    return 0;
}

int zm_msqueue_enqueue(zm_msqueue_t* q, void *data) {
    zm_ptr_t tail;
    zm_ptr_t next;
    zm_hzdptr_t *hzdptrs = zm_hzdptr_get();
    zm_msqnode_t* node = (zm_msqnode_t*) malloc(sizeof(zm_msqnode_t));
    node->data = data;
    zm_atomic_store(&node->next, ZM_NULL, zm_memord_release);
    while (1) {
        tail = (zm_ptr_t) zm_atomic_load(&q->tail, zm_memord_acquire);
        hzdptrs[0] = tail;
        if(tail == zm_atomic_load(&q->tail, zm_memord_acquire)) {
            next = (zm_ptr_t) zm_atomic_load(&((zm_msqnode_t*)tail)->next, zm_memord_acquire);
            if (next == ZM_NULL) {
                if (zm_atomic_compare_exchange_weak(&((zm_msqnode_t*)tail)->next,
                                                    &next,
                                                    (zm_ptr_t)node,
                                                    zm_memord_release,
                                                    zm_memord_acquire))
                    break;
            } else {
                zm_atomic_compare_exchange_weak(&q->tail,
                                                &tail,
                                                (zm_ptr_t)next,
                                                zm_memord_release,
                                                zm_memord_acquire);
            }
        }
    }
    zm_atomic_compare_exchange_weak(&q->tail,
                                    &tail,
                                    (zm_ptr_t)node,
                                    zm_memord_release,
                                    zm_memord_acquire);
    return 0;
}

int zm_msqueue_dequeue(zm_msqueue_t* q, void **data) {
    zm_ptr_t head;
    zm_ptr_t tail;
    zm_ptr_t next;
    zm_hzdptr_t *hzdptrs = zm_hzdptr_get();
    *data = NULL;
    while (1) {
        head = (zm_ptr_t) zm_atomic_load(&q->head, zm_memord_acquire);
        hzdptrs[0] = head;
        if(head == q->head) {
            tail = (zm_ptr_t) zm_atomic_load(&q->tail, zm_memord_acquire);
            next = (zm_ptr_t) zm_atomic_load(&((zm_msqnode_t*)head)->next, zm_memord_acquire);
            hzdptrs[1] = next;
            if (head == tail) {
                if (next == ZM_NULL) return 0;
                zm_atomic_compare_exchange_weak(&q->tail,
                                                &tail,
                                                (zm_ptr_t)next,
                                                zm_memord_release,
                                                zm_memord_acquire);
            } else {
                if (zm_atomic_compare_exchange_weak(&q->head,
                                                    &head,
                                                    (zm_ptr_t)next,
                                                    zm_memord_release,
                                                    zm_memord_acquire)) {
                    *data = ((zm_msqnode_t*)next)->data;
                    break;
                }
            }
        }
    }
    zm_hzdptr_retire(head);
    return 1;
}
