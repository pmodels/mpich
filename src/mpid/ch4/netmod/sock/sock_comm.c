/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "sock_impl.h"

int MPIDI_SOCK_mpi_comm_commit_pre_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    if (comm->attr & MPIR_COMM_ATTR__BOOTSTRAP) {
        mpi_errno = MPIDI_SOCK_comm_addr_exchange(comm);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_SOCK_mpi_comm_commit_post_hook(MPIR_Comm * comm)
{
    return MPI_SUCCESS;
}

int MPIDI_SOCK_mpi_comm_free_hook(MPIR_Comm * comm)
{
    return MPI_SUCCESS;
}

int MPIDI_SOCK_comm_set_hints(MPIR_Comm * comm, MPIR_Info * info)
{
    return MPI_SUCCESS;
}

int MPIDI_SOCK_comm_set_vcis(MPIR_Comm * comm, int num_implicit, int num_reserved,
                             MPIDI_num_vci_t * all_num_vcis)
{
    /* The sock netmod only supports a single vci. Every process runs the same
     * netmod, so there is no need to exchange the numbers. */
    for (int i = 0; i < comm->local_size; i++) {
        all_num_vcis[i].n_vcis = 1;
        all_num_vcis[i].n_total_vcis = 1;
    }

    return MPI_SUCCESS;
}
