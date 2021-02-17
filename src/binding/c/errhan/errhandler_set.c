/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Errhandler_set */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Errhandler_set = PMPI_Errhandler_set
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Errhandler_set  MPI_Errhandler_set
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Errhandler_set as PMPI_Errhandler_set
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Errhandler_set(MPI_Comm comm, MPI_Errhandler errhandler)
     __attribute__ ((weak, alias("PMPI_Errhandler_set")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Errhandler_set
#define MPI_Errhandler_set PMPI_Errhandler_set

#endif

/*@
   MPI_Errhandler_set - Sets the error handler for a communicator

Input Parameters:
+ comm - communicator (handle)
- errhandler - new error handler for communicator (handle)

.N Removed
   The replacement for this routine is 'MPI_Comm_set_errhandler'.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS

.seealso: MPI_Comm_set_errhandler, MPI_Errhandler_create, MPI_Comm_create_errhandler
@*/

int MPI_Errhandler_set(MPI_Comm comm, MPI_Errhandler errhandler)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_ERRHANDLER_SET);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_ERRHANDLER_SET);

    mpi_errno = PMPI_Comm_set_errhandler(comm, errhandler);

    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_ERRHANDLER_SET);
    return mpi_errno;
}
