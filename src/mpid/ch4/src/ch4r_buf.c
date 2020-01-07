/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "mpidimpl.h"
#include "ch4r_buf.h"

MPIDIU_buf_pool_t *MPIDIU_create_buf_pool(int num, int size)
{
    MPIDIU_buf_pool_t *buf_pool;

    buf_pool = MPIDIU_create_buf_pool_internal(num, size, NULL);

    return buf_pool;
}

void MPIDIU_destroy_buf_pool(MPIDIU_buf_pool_t * pool)
{
    int ret;

    if (pool->next)
        MPIDIU_destroy_buf_pool(pool->next);

    MPID_Thread_mutex_destroy(&pool->lock, &ret);
    MPL_free(pool->memory_region);
    MPL_free(pool);

}
