/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef CH4R_BUF_H_INCLUDED
#define CH4R_BUF_H_INCLUDED

#include "ch4_impl.h"
#include <pthread.h>

/*
   initial prototype of buffer pool.

   TODO:
   - align buffer region
   - add garbage collection
   - use huge pages
*/

MPIDIU_buf_pool_t *MPIDIU_create_buf_pool(int num, int size);
void MPIDIU_destroy_buf_pool(MPIDIU_buf_pool_t * pool);

static inline MPIDIU_buf_pool_t *MPIDIU_create_buf_pool_internal(int num, int size,
                                                                 MPIDIU_buf_pool_t * parent_pool)
{
    int i, ret;
    MPIDIU_buf_pool_t *buf_pool;
    MPIDIU_buf_t *curr, *next;




    buf_pool = (MPIDIU_buf_pool_t *) MPL_malloc(sizeof(*buf_pool), MPL_MEM_BUFFER);
    MPIR_Assert(buf_pool);
    MPID_Thread_mutex_create(&buf_pool->lock, &ret);

    buf_pool->size = size;
    buf_pool->num = num;
    buf_pool->next = NULL;
    buf_pool->memory_region = MPL_malloc(num * (sizeof(MPIDIU_buf_t) + size), MPL_MEM_BUFFER);
    MPIR_Assert(buf_pool->memory_region);

    curr = (MPIDIU_buf_t *) buf_pool->memory_region;
    buf_pool->head = curr;
    for (i = 0; i < num - 1; i++) {
        next = (MPIDIU_buf_t *) ((char *) curr + size + sizeof(MPIDIU_buf_t));
        curr->next = next;
        curr->pool = parent_pool ? parent_pool : buf_pool;
        curr = curr->next;
    }
    curr->next = NULL;
    curr->pool = parent_pool ? parent_pool : buf_pool;

    return buf_pool;
}

static inline void *MPIDIU_get_head_buf(MPIDIU_buf_pool_t * pool)
{
    void *buf;
    MPIDIU_buf_t *curr;



    curr = pool->head;
    pool->head = curr->next;
    buf = curr->data;


    return buf;
}

static inline void *MPIDIU_get_buf_safe(MPIDIU_buf_pool_t * pool)
{
    void *buf;
    MPIDIU_buf_pool_t *curr_pool;




    if (pool->head) {
        buf = MPIDIU_get_head_buf(pool);
        goto fn_exit;
    }

    curr_pool = pool;
    while (curr_pool->next)
        curr_pool = curr_pool->next;

    curr_pool->next = MPIDIU_create_buf_pool_internal(pool->num, pool->size, pool);
    MPIR_Assert(curr_pool->next);
    pool->head = curr_pool->next->head;
    buf = MPIDIU_get_head_buf(pool);

  fn_exit:

    return buf;
}

static inline void *MPIDIU_get_buf(MPIDIU_buf_pool_t * pool)
{
    void *buf;




    MPID_THREAD_CS_ENTER(VCI, pool->lock);
    buf = MPIDIU_get_buf_safe(pool);
    MPID_THREAD_CS_EXIT(VCI, pool->lock);


    return buf;
}

static inline void MPIDIU_release_buf_safe(void *buf)
{
    MPIDIU_buf_t *curr_buf;



    curr_buf = MPL_container_of(buf, MPIDIU_buf_t, data);
    curr_buf->next = curr_buf->pool->head;
    curr_buf->pool->head = curr_buf;


}

static inline void MPIDIU_release_buf(void *buf)
{
    MPIDIU_buf_t *curr_buf;



    curr_buf = MPL_container_of(buf, MPIDIU_buf_t, data);
    MPID_THREAD_CS_ENTER(VCI, curr_buf->pool->lock);
    curr_buf->next = curr_buf->pool->head;
    curr_buf->pool->head = curr_buf;
    MPID_THREAD_CS_EXIT(VCI, curr_buf->pool->lock);


}

#endif /* CH4R_BUF_H_INCLUDED */
