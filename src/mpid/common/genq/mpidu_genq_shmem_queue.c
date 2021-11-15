/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "mpidu_genq_shmem_pool.h"
#include "mpidu_genq_shmem_queue.h"
#include "mpidu_genqi_shmem_types.h"

int MPIDU_genq_shmem_queue_init(MPIDU_genq_shmem_queue_t queue, int flags)
{
    int rc = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIDU_genq_shmem_queue_u *queue_obj = (MPIDU_genq_shmem_queue_u *) queue;
    queue_obj->q.flags = flags;

    if (flags == MPIDU_GENQ_SHMEM_QUEUE_TYPE__SERIAL) {
        rc = MPIDU_genqi_serial_init(queue_obj);
    } else if (flags == MPIDU_GENQ_SHMEM_QUEUE_TYPE__INV_MPSC) {
        rc = MPIDU_genqi_inv_mpsc_init(queue_obj);
    } else if (flags == MPIDU_GENQ_SHMEM_QUEUE_TYPE__NEM_MPSC) {
        rc = MPIDU_genqi_nem_mpsc_init(queue_obj);
    } else if (flags == MPIDU_GENQ_SHMEM_QUEUE_TYPE__NEM_MPMC) {
        rc = MPIDU_genqi_nem_mpmc_init(queue_obj);
    } else {
        MPIR_Assert_error("Invalid GenQ flag");
    }

    MPIR_FUNC_EXIT;
    return rc;
}
