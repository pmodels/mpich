/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "posix_types.h"

int MPIDI_POSIX_comm_set_vcis(MPIR_Comm * comm, int num_vcis)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_POSIX_global.num_vcis = num_vcis;

    for (int i = 1; i < MPIDI_POSIX_global.num_vcis; i++) {
        mpi_errno = MPIDI_POSIX_init_vci(i);
        MPIR_ERR_CHECK(mpi_errno);
    }

    mpi_errno = MPIDI_POSIX_eager_post_init();
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
