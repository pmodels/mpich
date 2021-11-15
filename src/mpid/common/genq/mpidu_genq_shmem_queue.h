/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDU_GENQ_SHMEM_QUEUE_H_INCLUDED
#define MPIDU_GENQ_SHMEM_QUEUE_H_INCLUDED

#include "mpidimpl.h"
#include "mpidu_genqi_shmem_types.h"

#include <stdint.h>
#include <stdio.h>

#define MPIDU_GENQ_SHMEM_QUEUE_TYPE__MPSC MPIDU_GENQ_SHMEM_QUEUE_TYPE__NEM_MPSC
#define MPIDU_GENQ_SHMEM_QUEUE_TYPE__MPMC MPIDU_GENQ_SHMEM_QUEUE_TYPE__NEM_MPMC

typedef enum {
    MPIDU_GENQ_SHMEM_QUEUE_TYPE__SERIAL,
    MPIDU_GENQ_SHMEM_QUEUE_TYPE__INV_MPSC,
    MPIDU_GENQ_SHMEM_QUEUE_TYPE__NEM_MPSC,
    MPIDU_GENQ_SHMEM_QUEUE_TYPE__NEM_MPMC,
} MPIDU_genq_shmem_queue_type_e;

/* SERIAL */

static inline int MPIDU_genqi_serial_init(MPIDU_genq_shmem_queue_u * queue)
{
    queue->q.head.s = 0;
    queue->q.tail.s = 0;
    return 0;
}

static inline int MPIDU_genqi_serial_dequeue(MPIDU_genqi_shmem_pool_s * pool_obj,
                                             MPIDU_genq_shmem_queue_u * queue, void **cell)
{
    MPIDU_genqi_shmem_cell_header_s *cell_h = NULL;
    cell_h = HANDLE_TO_HEADER(pool_obj, queue->q.head.s);
    if (queue->q.head.s) {
        queue->q.head.s = cell_h->u.serial_queue.next;
        if (!cell_h->u.serial_queue.next) {
            queue->q.tail.s = 0;
        }
        *cell = HEADER_TO_CELL(cell_h);
    } else {
        *cell = NULL;
    }
    return 0;
}

static inline int MPIDU_genqi_serial_enqueue(MPIDU_genqi_shmem_pool_s * pool_obj,
                                             MPIDU_genq_shmem_queue_u * queue, void *cell)
{
    MPIDU_genqi_shmem_cell_header_s *cell_h = CELL_TO_HEADER(cell);
    cell_h->u.serial_queue.next = 0;

    uintptr_t handle = cell_h->handle;

    if (queue->q.tail.s) {
        HANDLE_TO_HEADER(pool_obj, queue->q.tail.s)->u.serial_queue.next = handle;
    }
    queue->q.tail.s = handle;
    if (!queue->q.head.s) {
        queue->q.head.s = queue->q.tail.s;
    }
    return 0;
}

/* NEMESIS MPSC QUEUE */

static inline int MPIDU_genqi_nem_mpsc_init(MPIDU_genq_shmem_queue_u * queue)
{
    MPL_atomic_store_ptr(&queue->q.head.m, NULL);
    MPL_atomic_store_ptr(&queue->q.tail.m, NULL);
    return 0;
}

static inline int MPIDU_genqi_nem_mpsc_dequeue(MPIDU_genqi_shmem_pool_s * pool_obj,
                                               MPIDU_genq_shmem_queue_u * queue, void **cell)
{
    void *handle = MPL_atomic_load_ptr(&queue->q.head.m);
    if (!handle) {
        /* queue is empty */
        *cell = NULL;
    } else {
        MPIDU_genqi_shmem_cell_header_s *cell_h = NULL;
        cell_h = HANDLE_TO_HEADER(pool_obj, handle);
        *cell = HEADER_TO_CELL(cell_h);

        void *next_handle = MPL_atomic_load_ptr(&cell_h->u.nem_queue.next_m);
        if (next_handle != NULL) {
            /* just dequeue the head */
            MPL_atomic_store_ptr(&queue->q.head.m, next_handle);
        } else {
            /* single element, tail == head,
             * have to make sure no enqueuing is in progress */
            MPL_atomic_store_ptr(&queue->q.head.m, NULL);
            if (MPL_atomic_cas_ptr(&queue->q.tail.m, handle, NULL) == handle) {
                /* no enqueuing in progress, we are done */
            } else {
                /* busy wait for the enqueuing to finish */
                do {
                    next_handle = MPL_atomic_load_ptr(&cell_h->u.nem_queue.next_m);
                } while (next_handle == NULL);
                /* then set the header */
                MPL_atomic_store_ptr(&queue->q.head.m, next_handle);
            }
        }
    }
    return 0;
}

static inline int MPIDU_genqi_nem_mpsc_enqueue(MPIDU_genqi_shmem_pool_s * pool_obj,
                                               MPIDU_genq_shmem_queue_u * queue, void *cell)
{
    MPIDU_genqi_shmem_cell_header_s *cell_h = CELL_TO_HEADER(cell);
    MPL_atomic_store_ptr(&cell_h->u.nem_queue.next_m, NULL);

    void *handle = (void *) cell_h->handle;

    void *tail_handle = NULL;
    tail_handle = MPL_atomic_swap_ptr(&queue->q.tail.m, handle);
    if (tail_handle == NULL) {
        /* queue was empty */
        MPL_atomic_store_ptr(&queue->q.head.m, handle);
    } else {
        MPIDU_genqi_shmem_cell_header_s *tail_cell_h = NULL;
        tail_cell_h = HANDLE_TO_HEADER(pool_obj, tail_handle);
        MPL_atomic_store_ptr(&tail_cell_h->u.nem_queue.next_m, handle);
    }
    return 0;
}

/* NEMESIS MPMC QUEUE */

static inline int MPIDU_genqi_nem_mpmc_init(MPIDU_genq_shmem_queue_u * queue)
{
    MPL_atomic_store_ptr(&queue->q.head.m, NULL);
    MPL_atomic_store_ptr(&queue->q.tail.m, NULL);
    return 0;
}

static inline int MPIDU_genqi_nem_mpmc_dequeue(MPIDU_genqi_shmem_pool_s * pool_obj,
                                               MPIDU_genq_shmem_queue_u * queue, void **cell)
{
    int counter = 0;

    /* Add an inner loop here to avoid going all the way around the progress engine in the case of
     * contention on this lock. If this is heavily contended, eventually this will give up and kick
     * back out to the full progress engine. */
    while (counter++ <= 20) {
        void *handle = MPL_atomic_load_ptr(&queue->q.head.m);
        if (!handle) {
            /* queue is empty */
            *cell = NULL;
            break;
        } else {
            MPIDU_genqi_shmem_cell_header_s *cell_h = NULL;
            cell_h = HANDLE_TO_HEADER(pool_obj, handle);
            *cell = HEADER_TO_CELL(cell_h);

            void *next_handle = MPL_atomic_load_ptr(&cell_h->u.nem_queue.next_m);
            if (next_handle != NULL) {
                /* just dequeue the head */
                if (MPL_atomic_cas_ptr(&queue->q.head.m, handle, next_handle) != handle) {
                    /* Multiple head dequeues at the same time. Give up holding the head of the queue
                     * and start over. */
                    *cell = NULL;
                    continue;
                }
            } else {
                /* single element, tail == head,
                 * have to make sure no enqueuing is in progress */
                if (MPL_atomic_cas_ptr(&queue->q.head.m, handle, NULL) != handle) {
                    /* Conflicts over head/tail pointers. Give up holding the head of the queue and
                     * start over. */
                    *cell = NULL;
                    continue;
                }
                if (MPL_atomic_cas_ptr(&queue->q.tail.m, handle, NULL) == handle) {
                    /* no enqueuing in progress, we are done */
                } else {
                    /* busy wait for the enqueuing to finish */
                    do {
                        next_handle = MPL_atomic_load_ptr(&cell_h->u.nem_queue.next_m);
                    } while (next_handle == NULL);
                    /* then set the header */
                    MPL_atomic_store_ptr(&queue->q.head.m, next_handle);
                }
            }
        }
    }

    return 0;
}

static inline int MPIDU_genqi_nem_mpmc_enqueue(MPIDU_genqi_shmem_pool_s * pool_obj,
                                               MPIDU_genq_shmem_queue_u * queue, void *cell)
{
    MPIDU_genqi_shmem_cell_header_s *cell_h = CELL_TO_HEADER(cell);
    MPL_atomic_store_ptr(&cell_h->u.nem_queue.next_m, NULL);

    void *handle = (void *) cell_h->handle;

    void *tail_handle = NULL;
    tail_handle = MPL_atomic_swap_ptr(&queue->q.tail.m, handle);
    if (tail_handle == NULL) {
        /* queue was empty */
        MPL_atomic_store_ptr(&queue->q.head.m, handle);
    } else {
        MPIDU_genqi_shmem_cell_header_s *tail_cell_h = NULL;
        tail_cell_h = HANDLE_TO_HEADER(pool_obj, tail_handle);
        MPL_atomic_store_ptr(&tail_cell_h->u.nem_queue.next_m, handle);
    }
    return 0;
}


/* INVERSE QUEUE */

static inline int MPIDU_genqi_inv_mpsc_init(MPIDU_genq_shmem_queue_u * queue)
{
    queue->q.head.s = 0;
    /* sp and mp all use atomic tail */
    MPL_atomic_store_ptr(&queue->q.tail.m, NULL);

    return 0;
}

static inline MPIDU_genqi_shmem_cell_header_s
    * MPIDU_genqi_shmem_get_head_cell_header(MPIDU_genqi_shmem_pool_s * pool_obj,
                                             MPIDU_genq_shmem_queue_u * queue)
{
    void *tail = NULL;
    MPIDU_genqi_shmem_cell_header_s *head_cell_h = NULL;

    /* prepares the cells for dequeuing from the head in the following steps.
     * 1. atomic detaching all cells frm the queue tail
     * 2. find the head of the queue and rebuild the "next" pointers for cells
     */
    tail = MPL_atomic_swap_ptr(&queue->q.tail.m, NULL);
    if (!tail) {
        return NULL;
    }
    head_cell_h = HANDLE_TO_HEADER(pool_obj, tail);

    if (head_cell_h != NULL) {
        uintptr_t curr_handle = head_cell_h->handle;
        while (head_cell_h->u.inverse_queue.prev) {
            MPIDU_genqi_shmem_cell_header_s *prev_cell_h;
            prev_cell_h = HANDLE_TO_HEADER(pool_obj, head_cell_h->u.inverse_queue.prev);
            prev_cell_h->u.inverse_queue.next = curr_handle;
            curr_handle = head_cell_h->u.inverse_queue.prev;
            head_cell_h = prev_cell_h;
        }
        return head_cell_h;
    } else {
        return NULL;
    }
}

static inline int MPIDU_genqi_inv_mpsc_dequeue(MPIDU_genqi_shmem_pool_s * pool_obj,
                                               MPIDU_genq_shmem_queue_u * queue, void **cell)
{
    int rc = MPI_SUCCESS;
    MPIDU_genqi_shmem_cell_header_s *cell_h = NULL;

    if (!queue->q.head.s) {
        cell_h = MPIDU_genqi_shmem_get_head_cell_header(pool_obj, queue);
        if (cell_h) {
            *cell = HEADER_TO_CELL(cell_h);
            queue->q.head.s = cell_h->u.inverse_queue.next;
        } else {
            *cell = NULL;
        }
    } else {
        cell_h = HANDLE_TO_HEADER(pool_obj, queue->q.head.s);
        *cell = HEADER_TO_CELL(cell_h);
        queue->q.head.s = cell_h->u.inverse_queue.next;
    }

    return rc;
}

static inline int MPIDU_genqi_inv_mpsc_enqueue(MPIDU_genqi_shmem_pool_s * pool_obj,
                                               MPIDU_genq_shmem_queue_u * queue, void *cell)
{
    int rc = MPI_SUCCESS;
    MPIDU_genqi_shmem_cell_header_s *cell_h = CELL_TO_HEADER(cell);
    cell_h->u.inverse_queue.next = 0;
    cell_h->u.inverse_queue.prev = 0;

    void *prev_handle = NULL;
    void *handle = (void *) cell_h->handle;

    do {
        prev_handle = MPL_atomic_load_ptr(&queue->q.tail.m);
        cell_h->u.inverse_queue.prev = (uintptr_t) prev_handle;
    } while (MPL_atomic_cas_ptr(&queue->q.tail.m, prev_handle, handle) != prev_handle);

    return rc;
}

/* EXTERNAL INTERFACE */

static inline int MPIDU_genq_shmem_queue_dequeue(MPIDU_genq_shmem_pool_t pool,
                                                 MPIDU_genq_shmem_queue_t queue, void **cell)
{
    int rc = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIDU_genqi_shmem_pool_s *pool_obj = (MPIDU_genqi_shmem_pool_s *) pool;
    MPIDU_genq_shmem_queue_u *queue_obj = (MPIDU_genq_shmem_queue_u *) queue;
    int flags = queue_obj->q.flags;
    if (flags == MPIDU_GENQ_SHMEM_QUEUE_TYPE__SERIAL) {
        rc = MPIDU_genqi_serial_dequeue(pool_obj, queue_obj, cell);
    } else if (flags == MPIDU_GENQ_SHMEM_QUEUE_TYPE__INV_MPSC) {
        rc = MPIDU_genqi_inv_mpsc_dequeue(pool_obj, queue_obj, cell);
    } else if (flags == MPIDU_GENQ_SHMEM_QUEUE_TYPE__NEM_MPSC) {
        rc = MPIDU_genqi_nem_mpsc_dequeue(pool_obj, queue_obj, cell);
    } else if (flags == MPIDU_GENQ_SHMEM_QUEUE_TYPE__NEM_MPMC) {
        rc = MPIDU_genqi_nem_mpmc_dequeue(pool_obj, queue_obj, cell);
    } else {
        MPIR_Assert_error("Invalid GenQ flag");
    }

    MPIR_FUNC_EXIT;
    return rc;
}

static inline int MPIDU_genq_shmem_queue_enqueue(MPIDU_genq_shmem_pool_t pool,
                                                 MPIDU_genq_shmem_queue_t queue, void *cell)
{
    int rc = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIDU_genqi_shmem_pool_s *pool_obj = (MPIDU_genqi_shmem_pool_s *) pool;
    MPIDU_genq_shmem_queue_u *queue_obj = (MPIDU_genq_shmem_queue_u *) queue;
    int flags = queue_obj->q.flags;
    if (flags == MPIDU_GENQ_SHMEM_QUEUE_TYPE__SERIAL) {
        rc = MPIDU_genqi_serial_enqueue(pool_obj, queue_obj, cell);
    } else if (flags == MPIDU_GENQ_SHMEM_QUEUE_TYPE__INV_MPSC) {
        rc = MPIDU_genqi_inv_mpsc_enqueue(pool_obj, queue_obj, cell);
    } else if (flags == MPIDU_GENQ_SHMEM_QUEUE_TYPE__NEM_MPSC) {
        rc = MPIDU_genqi_nem_mpsc_enqueue(pool_obj, queue_obj, cell);
    } else if (flags == MPIDU_GENQ_SHMEM_QUEUE_TYPE__NEM_MPMC) {
        rc = MPIDU_genqi_nem_mpmc_enqueue(pool_obj, queue_obj, cell);
    } else {
        MPIR_Assert_error("Invalid GenQ flag");
    }

    MPIR_FUNC_EXIT;
    return rc;
}

#endif /* ifndef MPIDU_GENQ_SHMEM_QUEUE_H_INCLUDED */
