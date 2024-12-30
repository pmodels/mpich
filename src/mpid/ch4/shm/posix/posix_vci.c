/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "posix_types.h"

int MPIDI_POSIX_comm_set_vcis(MPIR_Comm * comm, int num_vcis)
{
    int mpi_errno = MPI_SUCCESS;

    /* We only set up vcis once */
    MPIR_Assert(MPIDI_POSIX_global.num_vcis == 1);

    MPIR_Comm *node_comm = comm->node_comm;
    if (node_comm == NULL) {
        /* nothing to do if there is no local domain */
        goto fn_exit;
    }

    int max_vcis;
    mpi_errno = MPIR_Allreduce_impl(&num_vcis, &max_vcis, 1, MPI_INT, MPI_MAX, node_comm,
                                    MPIR_ERR_NONE);

    MPIDI_POSIX_global.num_vcis = max_vcis;

    for (int i = 1; i < MPIDI_POSIX_global.num_vcis; i++) {
        mpi_errno = MPIDI_POSIX_init_vci(i);
        MPIR_ERR_CHECK(mpi_errno);
    }

    mpi_errno = MPIDI_POSIX_eager_set_vcis(comm);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
