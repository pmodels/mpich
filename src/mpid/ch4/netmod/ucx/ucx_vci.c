/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ucx_impl.h"

int MPIDI_UCX_comm_set_vcis(MPIR_Comm * comm, int num_vcis, int *all_num_vcis)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_UCX_global.num_vcis = num_vcis;

    /* set up local vcis */
    for (int i = 1; i < MPIDI_UCX_global.num_vcis; i++) {
        mpi_errno = MPIDI_UCX_init_worker(i);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* UCX netmod only support the same number of vcis on all procs */
    for (int i = 0; i < comm->local_size; i++) {
        all_num_vcis[i] = num_vcis;
    }

    /* address exchange */
    if (num_vcis > 1) {
        mpi_errno = MPIDI_UCX_all_vcis_address_exchange();
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
