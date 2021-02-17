/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Errhandler_create */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Errhandler_create = PMPI_Errhandler_create
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Errhandler_create  MPI_Errhandler_create
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Errhandler_create as PMPI_Errhandler_create
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Errhandler_create(MPI_Comm_errhandler_function *comm_errhandler_fn,
                          MPI_Errhandler *errhandler)
                           __attribute__ ((weak, alias("PMPI_Errhandler_create")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Errhandler_create
#define MPI_Errhandler_create PMPI_Errhandler_create

#endif

/*@
   MPI_Errhandler_create - Creates an MPI-style errorhandler

Input Parameters:
. comm_errhandler_fn - user defined error handling procedure (function)

Output Parameters:
. errhandler - MPI error handler (handle)

.N Removed
   The replacement for this routine is 'MPI_Comm_create_errhandler'.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS

.seealso: MPI_Comm_create_errhandler, MPI_Errhandler_free
@*/

int MPI_Errhandler_create(MPI_Comm_errhandler_function *comm_errhandler_fn,
                          MPI_Errhandler *errhandler)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_ERRHANDLER_CREATE);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_ERRHANDLER_CREATE);

    mpi_errno = PMPI_Comm_create_errhandler(comm_errhandler_fn, errhandler);

    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_ERRHANDLER_CREATE);
    return mpi_errno;
}
