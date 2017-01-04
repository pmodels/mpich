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
#include "ch4i_util.h"
#include <pthread.h>

/*
   initial prototype of buffer pool.

   TODO:
   - align buffer region
   - add garbage collection
   - use huge pages
*/

static inline MPIU_buf_pool_t *MPIDI_create_buf_pool(int num, int size,
                                                     MPIU_buf_pool_t * parent_pool)
{
    int i;
    MPIU_buf_pool_t *buf_pool;
    MPIU_buf_t *curr, *next;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CREATE_BUF_POOL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CREATE_BUF_POOL);

    buf_pool = (MPIU_buf_pool_t *) MPL_malloc(sizeof(*buf_pool));
    MPIR_Assert(buf_pool);
    pthread_mutex_init(&buf_pool->lock, NULL);

    buf_pool->size = size;
    buf_pool->num = num;
    buf_pool->next = NULL;
    buf_pool->memory_region = MPL_malloc(num * (sizeof(MPIU_buf_t) + size));
    MPIR_Assert(buf_pool->memory_region);

    curr = (MPIU_buf_t *) buf_pool->memory_region;
    buf_pool->head = curr;
    for (i = 0; i < num - 1; i++) {
        next = (MPIU_buf_t *) ((char *) curr + size + sizeof(MPIU_buf_t));
        curr->next = next;
        curr->pool = parent_pool ? parent_pool : buf_pool;
        curr = curr->next;
    }
    curr->next = NULL;
    curr->pool = parent_pool ? parent_pool : buf_pool;
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CREATE_BUF_POOL);
    return buf_pool;
}

static inline MPIU_buf_pool_t *MPIDI_CH4U_create_buf_pool(int num, int size)
{
    MPIU_buf_pool_t *buf_pool;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_CREATE_BUF_POOL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_CREATE_BUF_POOL);

    buf_pool = MPIDI_create_buf_pool(num, size, NULL);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_CREATE_BUF_POOL);
    return buf_pool;
}

static inline void *MPIDI_CH4U_get_head_buf(MPIU_buf_pool_t * pool)
{
    void *buf;
    MPIU_buf_t *curr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_GET_HEAD_BUF);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_GET_HEAD_BUF);

    curr = pool->head;
    pool->head = curr->next;
    buf = curr->data;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_GET_HEAD_BUF);
    return buf;
}

static inline void *MPIDI_CH4R_get_buf_safe(MPIU_buf_pool_t * pool)
{
    void *buf;
    MPIU_buf_pool_t *curr_pool;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_GET_BUF_SAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4R_GET_BUF_SAFE);

    if (pool->head) {
        buf = MPIDI_CH4U_get_head_buf(pool);
        goto fn_exit;
    }

    curr_pool = pool;
    while (curr_pool->next)
        curr_pool = curr_pool->next;

    curr_pool->next = MPIDI_create_buf_pool(pool->num, pool->size, pool);
    MPIR_Assert(curr_pool->next);
    pool->head = curr_pool->next->head;
    buf = MPIDI_CH4U_get_head_buf(pool);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_GET_BUF_SAFE);
    return buf;
}


static inline void *MPIDI_CH4R_get_buf(MPIU_buf_pool_t * pool)
{
    void *buf;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_GET_BUF);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4R_GET_BUF);

    pthread_mutex_lock(&pool->lock);
    buf = MPIDI_CH4R_get_buf_safe(pool);
    pthread_mutex_unlock(&pool->lock);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_GET_BUF);
    return buf;
}

static inline void MPIDI_CH4R_release_buf_safe(void *buf)
{
    MPIU_buf_t *curr_buf;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_RELEASE_BUF_SAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4R_RELEASE_BUF_SAFE);

    curr_buf = MPL_container_of(buf, MPIU_buf_t, data);
    curr_buf->next = curr_buf->pool->head;
    curr_buf->pool->head = curr_buf;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_RELEASE_BUF_SAFE);
}

static inline void MPIDI_CH4R_release_buf(void *buf)
{
    MPIU_buf_t *curr_buf;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_RELEASE_BUF);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4R_RELEASE_BUF);

    curr_buf = MPL_container_of(buf, MPIU_buf_t, data);
    pthread_mutex_lock(&curr_buf->pool->lock);
    curr_buf->next = curr_buf->pool->head;
    curr_buf->pool->head = curr_buf;
    pthread_mutex_unlock(&curr_buf->pool->lock);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_RELEASE_BUF);
}


static inline void MPIDI_CH4R_destroy_buf_pool(MPIU_buf_pool_t * pool)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4R_DESTROY_BUF_POOL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4R_DESTROY_BUF_POOL);

    if (pool->next)
        MPIDI_CH4R_destroy_buf_pool(pool->next);

    MPL_free(pool->memory_region);
    MPL_free(pool);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4R_DESTROY_BUF_POOL);
}

#endif /* CH4R_BUF_H_INCLUDED */
