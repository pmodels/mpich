/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mpidi_ch3_impl.h"

/* This routine is called by MPID_Finalize to finalize the channel. */
int MPIDI_CH3_Finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    mpi_errno = MPIDI_CH3I_Progress_finalize();
    MPIR_ERR_CHECK(mpi_errno);

  fn_fail:
    MPIR_FUNC_EXIT;
    return mpi_errno;
}
