/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

int MPIR_get_local_gpu_stream(MPIR_Comm * comm_ptr, MPL_gpu_stream_t * gpu_stream)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Stream *stream_ptr = MPIR_stream_comm_get_local_stream(comm_ptr);

    MPIR_ERR_CHKANDJUMP(!stream_ptr || stream_ptr->type != MPIR_STREAM_GPU,
                        mpi_errno, MPI_ERR_OTHER, "**notgpustream");
    *gpu_stream = stream_ptr->u.gpu_stream;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_allocate_enqueue_request(MPIR_Comm * comm_ptr, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Stream *stream_ptr = MPIR_stream_comm_get_local_stream(comm_ptr);
    MPIR_ERR_CHKANDJUMP(!stream_ptr || stream_ptr->type != MPIR_STREAM_GPU,
                        mpi_errno, MPI_ERR_OTHER, "**notgpustream");

    int vci = stream_ptr->vci;
    MPIR_Assert(vci > 0);

    /* stream vci are only accessed within a serialized context */
    (*req) = MPIR_Request_create_from_pool_safe(MPIR_REQUEST_KIND__ENQUEUE, vci, 1);
    (*req)->u.enqueue.stream_ptr = stream_ptr;
    (*req)->u.enqueue.real_request = NULL;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
