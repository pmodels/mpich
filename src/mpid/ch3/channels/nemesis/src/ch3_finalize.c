/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpid_nem_impl.h"

int MPIDI_CH3_Finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    mpi_errno = MPIDI_CH3I_Progress_finalize();
    MPIR_ERR_CHECK(mpi_errno);
    
    mpi_errno = MPID_nem_finalize();
    if (mpi_errno) MPIR_ERR_POP (mpi_errno);

 fn_fail:
    MPIR_FUNC_EXIT;
    return mpi_errno;
}
