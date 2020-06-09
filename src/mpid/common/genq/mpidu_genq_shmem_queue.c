/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "mpidu_genq_shmem_pool.h"
#include "mpidu_genq_shmem_queue.h"
#include "mpidu_genqi_shmem_types.h"

int MPIDU_genq_shmem_queue_init(MPIDU_genq_shmem_queue_t queue, MPIDU_genq_shmem_pool_t pool,
                                int flags)
{
    int rc = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDU_GENQ_SHMEM_QUEUE_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDU_GENQ_SHMEM_QUEUE_INIT);

    queue->q.pool = (MPIDU_genqi_shmem_pool_s *) pool;
    queue->q.flags = flags;

    if (flags & MPIDU_GENQ_SHMEM_QUEUE_TYPE__SERIAL) {
        queue->q.head.s = 0;
        queue->q.tail.s = 0;
    } else {
        queue->q.head.s = 0;
        /* sp and mp all use atomic tail */
        MPL_atomic_store_ptr(&queue->q.tail.m, NULL);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDU_GENQ_SHMEM_QUEUE_INIT);
    return rc;
}
