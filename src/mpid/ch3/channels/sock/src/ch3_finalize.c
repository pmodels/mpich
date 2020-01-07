/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

/* This routine is called by MPID_Finalize to finalize the channel. */
int MPIDI_CH3_Finalize(void)
{
    int mpi_errno = MPI_SUCCESS;




    mpi_errno = MPIDI_CH3I_Progress_finalize();
    MPIR_ERR_CHECK(mpi_errno);

  fn_fail:

    return mpi_errno;
}
