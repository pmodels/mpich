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

/* Turn a normal communicator into a stream communicator */
int MPIR_Comm_set_stream(MPIR_Comm * comm_ptr, MPIR_Stream * stream_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    int vci, *vci_table;
    if (stream_ptr) {
        vci = stream_ptr->vci;
    } else {
        vci = 0;
    }
    vci_table = MPL_malloc(comm_ptr->local_size * sizeof(int), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!vci_table, mpi_errno, MPI_ERR_OTHER, "**nomem");

    mpi_errno = MPIR_Allgather_impl(&vci, 1, MPIR_INT_INTERNAL,
                                    vci_table, 1, MPIR_INT_INTERNAL, comm_ptr, MPIR_ERR_NONE);
    MPIR_ERR_CHECK(mpi_errno);

    bool has_nonzero_vci = false;
    for (int i = 0; i < comm_ptr->local_size; i++) {
        if (vci_table[i] > 0) {
            has_nonzero_vci = true;
            break;
        }
    }

    if (has_nonzero_vci) {
        /* turn comm_ptr into a stream communicator */
        comm_ptr->stream_comm_type = MPIR_STREAM_COMM_SINGLE;
        comm_ptr->stream_comm.single.stream = stream_ptr;
        comm_ptr->stream_comm.single.vci_table = vci_table;
        if (stream_ptr) {
            MPIR_Object_add_ref(stream_ptr);
        }
    } else {
        /* fallback to a normal communicator */
        MPL_free(vci_table);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
