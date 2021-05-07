/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"


void MPIR_REPLACE(void *invec, void *inoutvec, MPI_Aint * Len, MPI_Datatype * type)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Localcopy(invec, *Len, *type, inoutvec, *Len, *type);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


int MPIR_REPLACE_check_dtype(MPI_Datatype type)
{
    return MPI_SUCCESS;
}
