/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ucx_impl.h"
#ifdef HAVE_HCOLL
#include "../../common/hcoll/hcoll.h"
#endif

int MPIDI_UCX_mpi_comm_commit_pre_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    if (comm == MPIR_Process.comm_world) {
        mpi_errno = MPIDI_UCX_comm_addr_exchange(MPIR_Process.comm_world);
        MPIR_ERR_CHECK(mpi_errno);
    }
#if defined HAVE_HCOLL
    hcoll_comm_create(comm, NULL);
#endif

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_UCX_mpi_comm_commit_post_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDI_UCX_mpi_comm_free_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

#ifdef HAVE_HCOLL
    hcoll_comm_destroy(comm, NULL);
#endif
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDI_UCX_comm_set_hints(MPIR_Comm * comm, MPIR_Info * info)
{
    int mpi_errno = MPI_SUCCESS;

    return mpi_errno;
}
