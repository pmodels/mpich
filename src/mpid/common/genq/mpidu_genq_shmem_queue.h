/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDU_GENQ_SHMEM_QUEUE_H_INCLUDED
#define MPIDU_GENQ_SHMEM_QUEUE_H_INCLUDED

#include "mpidimpl.h"
#include "mpidu_genq_shmem_pool.h"
#include "mpidu_genqi_shmem_types.h"
#include "mpidu_init_shm.h"
#include <stdint.h>
#include <stdio.h>

#define MPIDU_GENQ_SHMEM_QUEUE_TYPE__MPSC MPIDU_GENQ_SHMEM_QUEUE_TYPE__INV_MPSC

typedef enum {
    MPIDU_GENQ_SHMEM_QUEUE_TYPE__SERIAL,
    MPIDU_GENQ_SHMEM_QUEUE_TYPE__INV_MPSC,
    MPIDU_GENQ_SHMEM_QUEUE_TYPE__NEM_MPSC,
} MPIDU_genq_shmem_queue_type_e;

typedef union MPIDU_genq_shmem_queue {
    struct {
        union {
            uintptr_t s;
            MPL_atomic_ptr_t m;
            uint8_t pad[MPIDU_SHM_CACHE_LINE_LEN];
        } head;
        union {
            uintptr_t s;
            MPL_atomic_ptr_t m;
            uint8_t pad[MPIDU_SHM_CACHE_LINE_LEN];
        } tail;
        unsigned flags;
    } q;
    uint8_t pad[3 * MPIDU_SHM_CACHE_LINE_LEN];
} MPIDU_genq_shmem_queue_u;

typedef MPIDU_genq_shmem_queue_u *MPIDU_genq_shmem_queue_t;

int MPIDU_genq_shmem_queue_init(MPIDU_genq_shmem_queue_t queue, int flags);

/* SERIAL */

static inline int MPIDU_genqi_serial_init(MPIDU_genq_shmem_queue_t queue)
{
    queue->q.head.s = 0;
    queue->q.tail.s = 0;
    return 0;
}

static inline int MPIDU_genqi_serial_dequeue(MPIDU_genqi_shmem_pool_s * pool_obj,
                                             MPIDU_genq_shmem_queue_t queue, void **cell)
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
                                             MPIDU_genq_shmem_queue_t queue, void *cell)
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

/* NEMESIS QUEUE */

static inline int MPIDU_genqi_nem_mpsc_init(MPIDU_genq_shmem_queue_t queue)
{
    return 0;
}

static inline int MPIDU_genqi_nem_mpsc_dequeue(MPIDU_genqi_shmem_pool_s * pool_obj,
                                               MPIDU_genq_shmem_queue_t queue, void **cell)
{
    return 0;
}

static inline int MPIDU_genqi_nem_mpsc_enqueue(MPIDU_genqi_shmem_pool_s * pool_obj,
                                               MPIDU_genq_shmem_queue_t queue, void *cell)
{
    return 0;
}

/* INVERSE QUEUE */

static inline void *MPIDU_genq_shmem_queue_head(MPIDU_genq_shmem_queue_t queue);
static inline void *MPIDU_genq_shmem_queue_next(void *cell);

#define MPIDU_GENQ_SHMEM_QUEUE_FOREACH(queue, cell) \
    for (void *tmp = MPIDU_genq_shmem_queue_head((queue)), cell = tmp; tmp; \
         tmp = MPIDU_genq_shmem_queue_next(tmp))

static inline int MPIDU_genqi_inv_mpsc_init(MPIDU_genq_shmem_queue_t queue)
{
    queue->q.head.s = 0;
    /* sp and mp all use atomic tail */
    MPL_atomic_store_ptr(&queue->q.tail.m, NULL);

    return 0;
}

static inline MPIDU_genqi_shmem_cell_header_s
    * MPIDU_genqi_shmem_get_head_cell_header(MPIDU_genqi_shmem_pool_s * pool_obj,
                                             MPIDU_genq_shmem_queue_t queue)
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
                                               MPIDU_genq_shmem_queue_t queue, void **cell)
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

  fn_exit:
    return rc;
}

static inline int MPIDU_genqi_inv_mpsc_enqueue(MPIDU_genqi_shmem_pool_s * pool_obj,
                                               MPIDU_genq_shmem_queue_t queue, void *cell)
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

static inline void *MPIDU_genq_shmem_queue_head(MPIDU_genq_shmem_queue_t queue)
{
    MPIDU_genqi_shmem_cell_header_s *cell_h = NULL;

    if (queue->q.flags == MPIDU_GENQ_SHMEM_QUEUE_TYPE__SERIAL) {
        cell_h = HANDLE_TO_HEADER(queue->q.pool, queue->q.head.s);
    } else {    /* MPIDU_GENQ_SHMEM_QUEUE_TYPE__MPSC */
        if (!queue->q.head.s) {
            cell_h = MPIDU_genqi_shmem_get_head_cell_header(queue);
        } else {
            cell_h = HANDLE_TO_HEADER(queue->q.pool, queue->q.head.s);
        }

    }
    if (cell_h) {
        return HEADER_TO_CELL(cell_h);
    } else {
        return NULL;
    }
}

static inline void *MPIDU_genq_shmem_queue_next(void *cell)
{
    return HEADER_TO_CELL(CELL_TO_HEADER(cell)->next);
}

/* EXTERNAL INTERFACE */

static inline int MPIDU_genq_shmem_queue_dequeue(MPIDU_genq_shmem_pool_t pool,
                                                 MPIDU_genq_shmem_queue_t queue, void **cell)
{
    int rc = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDU_GENQ_SHMEM_QUEUE_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDU_GENQ_SHMEM_QUEUE_INIT);

    MPIDU_genqi_shmem_pool_s *pool_obj = (MPIDU_genqi_shmem_pool_s *) pool;
    int flags = queue->q.flags;
    if (flags == MPIDU_GENQ_SHMEM_QUEUE_TYPE__SERIAL) {
        rc = MPIDU_genqi_serial_dequeue(pool_obj, queue, cell);
    } else if (flags == MPIDU_GENQ_SHMEM_QUEUE_TYPE__INV_MPSC) {
        rc = MPIDU_genqi_inv_mpsc_dequeue(pool_obj, queue, cell);
    } else if (flags == MPIDU_GENQ_SHMEM_QUEUE_TYPE__MPSC) {
        rc = MPIDU_genqi_nem_mpsc_dequeue(pool_obj, queue, cell);
    } else {
        MPIR_Assert(0 && "Invalid GenQ flag");
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDU_GENQ_SHMEM_QUEUE_INIT);
    return rc;
}

static inline int MPIDU_genq_shmem_queue_enqueue(MPIDU_genq_shmem_pool_t pool,
                                                 MPIDU_genq_shmem_queue_t queue, void *cell)
{
    int rc = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDU_GENQ_SHMEM_QUEUE_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDU_GENQ_SHMEM_QUEUE_INIT);

    MPIDU_genqi_shmem_pool_s *pool_obj = (MPIDU_genqi_shmem_pool_s *) pool;
    int flags = queue->q.flags;
    if (flags == MPIDU_GENQ_SHMEM_QUEUE_TYPE__SERIAL) {
        rc = MPIDU_genqi_serial_enqueue(pool_obj, queue, cell);
    } else if (flags == MPIDU_GENQ_SHMEM_QUEUE_TYPE__INV_MPSC) {
        rc = MPIDU_genqi_inv_mpsc_enqueue(pool_obj, queue, cell);
    } else if (flags == MPIDU_GENQ_SHMEM_QUEUE_TYPE__MPSC) {
        rc = MPIDU_genqi_nem_mpsc_enqueue(pool_obj, queue, cell);
    } else {
        MPIR_Assert(0 && "Invalid GenQ flag");
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDU_GENQ_SHMEM_QUEUE_INIT);
    return rc;
}

#endif /* ifndef MPIDU_GENQ_SHMEM_QUEUE_H_INCLUDED */
