/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

#ifdef ENABLE_CCLCOMM

int MPIR_CCL_check_both_gpu_bufs(const void *sendbuf, void *recvbuf)
{
    MPL_pointer_attr_t recv_attr;

    int mpi_errno = MPI_SUCCESS;
    MPL_gpu_query_pointer_attr(recvbuf, &recv_attr);
    if (mpi_errno != MPL_SUCCESS) {
        return 0;
    }
    if (recv_attr.type != MPL_GPU_POINTER_DEV) {
        return 0;
    }

    if (sendbuf != MPI_IN_PLACE) {
        MPL_pointer_attr_t send_attr;
        MPL_gpu_query_pointer_attr(sendbuf, &send_attr);
        if (mpi_errno != MPL_SUCCESS) {
            return 0;
        }
        if (send_attr.type != MPL_GPU_POINTER_DEV) {
            return 0;
        }
    }

    return 1;
}

#endif /* ENABLE_CCLCOMM */
