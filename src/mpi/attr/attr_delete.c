/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Attr_delete */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Attr_delete = PMPI_Attr_delete
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Attr_delete  MPI_Attr_delete
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Attr_delete as PMPI_Attr_delete
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Attr_delete(MPI_Comm comm, int keyval) __attribute__ ((weak, alias("PMPI_Attr_delete")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Attr_delete
#define MPI_Attr_delete PMPI_Attr_delete

#endif

/*@

MPI_Attr_delete - Deletes an attribute value associated with a key on a
    communicator

Input Parameters:
+ comm - communicator to which attribute is attached (handle)
- keyval - The key value of the deleted attribute (integer)

.N ThreadSafe

.N Deprecated
   The replacement for this routine is 'MPI_Comm_delete_attr'.

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_PERM_KEY
@*/
int MPI_Attr_delete(MPI_Comm comm, int keyval)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_ATTR_DELETE);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_ATTR_DELETE);

    mpi_errno = PMPI_Comm_delete_attr(comm, keyval);

    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_ATTR_DELETE);
    return mpi_errno;
}
