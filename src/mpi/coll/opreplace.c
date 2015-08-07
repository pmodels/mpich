/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"


#undef FUNCNAME
#define FUNCNAME MPIR_REPLACE
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
void MPIR_REPLACE( void *invec, void *inoutvec, int *Len, MPI_Datatype *type )
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Localcopy(invec, *Len, *type, inoutvec, *Len, *type);
    if (mpi_errno) { MPIR_ERR_POP(mpi_errno); }

  fn_exit:
    return;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIR_REPLACE_check_dtype
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_REPLACE_check_dtype( MPI_Datatype type )
{
    return MPI_SUCCESS;
}
