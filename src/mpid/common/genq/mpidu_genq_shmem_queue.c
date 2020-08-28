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

    if (flags == MPIDU_GENQ_SHMEM_QUEUE_TYPE__SERIAL) {
        rc = MPIDU_genqi_serial_init(queue);
    } else if (flags == MPIDU_GENQ_SHMEM_QUEUE_TYPE__INV_MPSC) {
        rc = MPIDU_genqi_inv_mpsc_init(queue);
    } else if (flags == MPIDU_GENQ_SHMEM_QUEUE_TYPE__NEM_MPSC) {
        rc = MPIDU_genqi_nem_mpsc_init(queue);
    } else {
        MPIR_Assert(0 && "Invalid GenQ flag");
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDU_GENQ_SHMEM_QUEUE_INIT);
    return rc;
}
