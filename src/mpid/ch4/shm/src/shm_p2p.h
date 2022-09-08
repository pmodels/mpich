/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef SHM_P2P_H_INCLUDED
#define SHM_P2P_H_INCLUDED

#include <shm.h>
#include "../ipc/src/shm_inline.h"
#include "../posix/shm_inline.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_isend(const void *buf, MPI_Aint count,
                                                 MPI_Datatype datatype, int rank, int tag,
                                                 MPIR_Comm * comm, int attr,
                                                 MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    mpi_errno = MPIDI_IPC_mpi_isend(buf, count, datatype, rank, tag, comm, attr, addr, request);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_cancel_send(MPIR_Request * sreq)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_cancel_send(sreq);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_irecv(void *buf, MPI_Aint count, MPI_Datatype datatype,
                                                 int rank, int tag, MPIR_Comm * comm,
                                                 int context_offset, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno =
        MPIDI_POSIX_mpi_irecv(buf, count, datatype, rank, tag, comm, context_offset, request);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_imrecv(void *buf, MPI_Aint count, MPI_Datatype datatype,
                                                  MPIR_Request * message)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    mpi_errno = MPIDI_POSIX_mpi_imrecv(buf, count, datatype, message);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_cancel_recv(MPIR_Request * rreq)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_cancel_recv(rreq);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_improbe(int source, int tag, MPIR_Comm * comm,
                                                   int context_offset, int *flag,
                                                   MPIR_Request ** message, MPI_Status * status)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_improbe(source, tag, comm, context_offset, flag, message, status);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_iprobe(int source, int tag, MPIR_Comm * comm,
                                                  int context_offset, int *flag,
                                                  MPI_Status * status)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_iprobe(source, tag, comm, context_offset, flag, status);

    MPIR_FUNC_EXIT;
    return ret;
}

#endif /* SHM_P2P_H_INCLUDED */
