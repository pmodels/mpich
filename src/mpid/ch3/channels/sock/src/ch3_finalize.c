/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidi_ch3_impl.h"

/* This routine is called by MPID_Finalize to finalize the channel. */
int MPIDI_CH3_Finalize(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_CH3_FINALIZE);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_CH3_FINALIZE);

    mpi_errno = MPIDI_CH3I_Progress_finalize();
    MPIR_ERR_CHECK(mpi_errno);

  fn_fail:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_CH3_FINALIZE);
    return mpi_errno;
}
