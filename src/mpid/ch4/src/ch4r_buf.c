/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#include "mpidimpl.h"
#include "ch4r_buf.h"

MPIDIU_buf_pool_t *MPIDIU_create_buf_pool(int num, int size)
{
    MPIDIU_buf_pool_t *buf_pool;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_CREATE_BUF_POOL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_CREATE_BUF_POOL);

    buf_pool = MPIDIU_create_buf_pool_internal(num, size, NULL);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_CREATE_BUF_POOL);
    return buf_pool;
}

void MPIDIU_destroy_buf_pool(MPIDIU_buf_pool_t * pool)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_DESTROY_BUF_POOL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_DESTROY_BUF_POOL);

    if (pool->next)
        MPIDIU_destroy_buf_pool(pool->next);

    MPID_Thread_mutex_destroy(&pool->lock, &ret);
    MPL_free(pool->memory_region);
    MPL_free(pool);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_DESTROY_BUF_POOL);
}
