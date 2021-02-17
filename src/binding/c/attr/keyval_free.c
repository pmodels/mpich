/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Keyval_free */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Keyval_free = PMPI_Keyval_free
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Keyval_free  MPI_Keyval_free
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Keyval_free as PMPI_Keyval_free
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Keyval_free(int *keyval)  __attribute__ ((weak, alias("PMPI_Keyval_free")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Keyval_free
#define MPI_Keyval_free PMPI_Keyval_free

#endif

/*@
   MPI_Keyval_free - Frees an attribute key for communicators

Input/Output Parameters:
. keyval - Frees the integer key value (integer)

.N Deprecated
   The replacement for this routine is 'MPI_Comm_free_keyval'.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS

.seealso: MPI_Keyval_create, MPI_Comm_free_keyval
@*/

int MPI_Keyval_free(int *keyval)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_KEYVAL_FREE);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_KEYVAL_FREE);

    mpi_errno = PMPI_Comm_free_keyval(keyval);

    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_KEYVAL_FREE);
    return mpi_errno;
}
