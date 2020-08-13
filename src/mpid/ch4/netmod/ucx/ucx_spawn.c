/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ucx_impl.h"

int MPIDI_UCX_mpi_open_port(MPIR_Info * info_ptr, char *port_name)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**ucx_nm_notsupported");

    return mpi_errno;
}

int MPIDI_UCX_mpi_close_port(const char *port_name)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**ucx_nm_notsupported");

    return mpi_errno;
}

int MPIDI_UCX_mpi_comm_connect(const char *port_name, MPIR_Info * info, int root, int timeout,
                               MPIR_Comm * comm_ptr, MPIR_Comm ** newcomm)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**ucx_nm_notsupported");

    return mpi_errno;
}

int MPIDI_UCX_mpi_comm_disconnect(MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_UCX_MPI_COMM_DISCONNECT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_UCX_MPI_COMM_DISCONNECT);

    mpi_errno = MPIR_Comm_free_impl(comm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_UCX_MPI_COMM_DISCONNECT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_UCX_mpi_comm_accept(const char *port_name, MPIR_Info * info, int root,
                              MPIR_Comm * comm_ptr, MPIR_Comm ** newcomm)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**ucx_nm_notsupported");

    return mpi_errno;
}
