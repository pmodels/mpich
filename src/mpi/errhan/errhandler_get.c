/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Errhandler_get */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Errhandler_get = PMPI_Errhandler_get
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Errhandler_get  MPI_Errhandler_get
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Errhandler_get as PMPI_Errhandler_get
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Errhandler_get(MPI_Comm comm, MPI_Errhandler * errhandler)
    __attribute__ ((weak, alias("PMPI_Errhandler_get")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Errhandler_get
#define MPI_Errhandler_get PMPI_Errhandler_get

#endif

/*@
  MPI_Errhandler_get - Gets the error handler for a communicator

Input Parameters:
. comm - communicator to get the error handler from (handle)

Output Parameters:
. errhandler - MPI error handler currently associated with communicator
(handle)

.N Removed
The replacement routine for this function is 'MPI_Comm_get_errhandler'.

.N ThreadSafe

.N Fortran

Note on Implementation:

The MPI Standard was unclear on whether this routine required the user to call
'MPI_Errhandler_free' once for each call made to this routine in order to
free the error handler.  After some debate, the MPI Forum added an explicit
statement that users are required to call 'MPI_Errhandler_free' when the
return value from this routine is no longer needed.  This behavior is similar
to the other MPI routines for getting objects; for example, 'MPI_Comm_group'
requires that the user call 'MPI_Group_free' when the group returned
by 'MPI_Comm_group' is no longer needed.

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_ARG
@*/
int MPI_Errhandler_get(MPI_Comm comm, MPI_Errhandler * errhandler)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_ERRHANDLER_GET);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_ERRHANDLER_GET);

    mpi_errno = PMPI_Comm_get_errhandler(comm, errhandler);

    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_ERRHANDLER_GET);
    return mpi_errno;
}
