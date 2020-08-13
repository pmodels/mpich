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

typedef enum {
    MPIDU_GENQ_SHMEM_QUEUE_TYPE__SERIAL,
    MPIDU_GENQ_SHMEM_QUEUE_TYPE__MPSC,
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
        MPIDU_genqi_shmem_pool_s *pool;
        unsigned flags;
    } q;
    uint8_t pad[3 * MPIDU_SHM_CACHE_LINE_LEN];
} MPIDU_genq_shmem_queue_u;

typedef MPIDU_genq_shmem_queue_u *MPIDU_genq_shmem_queue_t;

int MPIDU_genq_shmem_queue_init(MPIDU_genq_shmem_queue_t queue, MPIDU_genq_shmem_pool_t pool,
                                int flags);
static inline int MPIDU_genq_shmem_queue_dequeue(MPIDU_genq_shmem_queue_t queue, void **cell);
static inline int MPIDU_genq_shmem_queue_enqueue(MPIDU_genq_shmem_queue_t queue, void *cell);
static inline void *MPIDU_genq_shmem_queue_head(MPIDU_genq_shmem_queue_t queue);
static inline void *MPIDU_genq_shmem_queue_next(void *cell);

static inline MPIDU_genqi_shmem_cell_header_s
    * MPIDU_genqi_shmem_get_head_cell_header(MPIDU_genq_shmem_queue_t queue);

#define MPIDU_GENQ_SHMEM_QUEUE_FOREACH(queue, cell) \
    for (void *tmp = MPIDU_genq_shmem_queue_head((queue)), cell = tmp; tmp; \
         tmp = MPIDU_genq_shmem_queue_next(tmp))

static inline MPIDU_genqi_shmem_cell_header_s
    * MPIDU_genqi_shmem_get_head_cell_header(MPIDU_genq_shmem_queue_t queue)
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
    head_cell_h = HANDLE_TO_HEADER(queue->q.pool, tail);

    if (head_cell_h != NULL) {
        uintptr_t curr_handle = head_cell_h->handle;
        while (head_cell_h->prev) {
            MPIDU_genqi_shmem_cell_header_s *prev_cell_h = HANDLE_TO_HEADER(queue->q.pool,
                                                                            head_cell_h->prev);
            prev_cell_h->next = curr_handle;
            curr_handle = head_cell_h->prev;
            head_cell_h = prev_cell_h;
        }
        return head_cell_h;
    } else {
        return NULL;
    }
}

static inline int MPIDU_genq_shmem_queue_dequeue(MPIDU_genq_shmem_queue_t queue, void **cell)
{
    int rc = MPI_SUCCESS;
    MPIDU_genqi_shmem_cell_header_s *cell_h = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDU_GENQ_SHMEM_QUEUE_DEQUEUE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDU_GENQ_SHMEM_QUEUE_DEQUEUE);

    if (queue->q.flags == MPIDU_GENQ_SHMEM_QUEUE_TYPE__SERIAL) {
        cell_h = HANDLE_TO_HEADER(queue->q.pool, queue->q.head.s);
        if (queue->q.head.s) {
            queue->q.head.s = cell_h->next;
            if (!cell_h->next) {
                queue->q.tail.s = 0;
            }
            *cell = HEADER_TO_CELL(cell_h);
        } else {
            *cell = NULL;
        }
    } else {    /* MPIDU_GENQ_SHMEM_QUEUE_TYPE__MPSC */
        if (!queue->q.head.s) {
            cell_h = MPIDU_genqi_shmem_get_head_cell_header(queue);
            if (cell_h) {
                *cell = HEADER_TO_CELL(cell_h);
                queue->q.head.s = cell_h->next;
            } else {
                *cell = NULL;
            }
        } else {
            cell_h = HANDLE_TO_HEADER(queue->q.pool, queue->q.head.s);
            *cell = HEADER_TO_CELL(cell_h);
            queue->q.head.s = cell_h->next;
        }
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDU_GENQ_SHMEM_QUEUE_DEQUEUE);
    return rc;
}

static inline int MPIDU_genq_shmem_queue_enqueue(MPIDU_genq_shmem_queue_t queue, void *cell)
{
    int rc = MPI_SUCCESS;
    MPIDU_genqi_shmem_cell_header_s *cell_h = CELL_TO_HEADER(cell);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDU_GENQ_SHMEM_QUEUE_ENQUEUE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDU_GENQ_SHMEM_QUEUE_ENQUEUE);

    if (queue->q.flags == MPIDU_GENQ_SHMEM_QUEUE_TYPE__SERIAL) {
        uintptr_t handle = cell_h->handle;

        if (queue->q.tail.s) {
            HANDLE_TO_HEADER(queue->q.pool, queue->q.tail.s)->next = handle;
        }
        cell_h->prev = queue->q.tail.s;
        queue->q.tail.s = handle;
        if (!queue->q.head.s) {
            queue->q.head.s = queue->q.tail.s;
        }
    } else {    /* MPIDU_GENQ_SHMEM_QUEUE_TYPE__MPSC */
        void *prev_handle = NULL;
        void *handle = (void *) cell_h->handle;

        do {
            prev_handle = MPL_atomic_load_ptr(&queue->q.tail.m);
            cell_h->prev = (uintptr_t) prev_handle;
        } while (MPL_atomic_cas_ptr(&queue->q.tail.m, prev_handle, handle) != prev_handle);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDU_GENQ_SHMEM_QUEUE_ENQUEUE);
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

#endif /* ifndef MPIDU_GENQ_SHMEM_QUEUE_H_INCLUDED */
