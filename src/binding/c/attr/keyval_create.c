/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Keyval_create */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Keyval_create = PMPI_Keyval_create
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Keyval_create  MPI_Keyval_create
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Keyval_create as PMPI_Keyval_create
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Keyval_create(MPI_Copy_function *copy_fn, MPI_Delete_function *delete_fn, int *keyval,
                      void *extra_state)  __attribute__ ((weak, alias("PMPI_Keyval_create")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Keyval_create
#define MPI_Keyval_create PMPI_Keyval_create

#endif

/*@
   MPI_Keyval_create - Creates a new attribute key

Input Parameters:
+ copy_fn - Copy callback function for keyval (function)
. delete_fn - Delete callback function for keyval (function)
- extra_state - Extra state for callback functions (None)

Output Parameters:
. keyval - key value for future access (integer)

.N Deprecated
   The replacement for this routine is 'MPI_Comm_create_keyval'.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS

.seealso: MPI_Keyval_free, MPI_Comm_create_keyval
@*/

int MPI_Keyval_create(MPI_Copy_function *copy_fn, MPI_Delete_function *delete_fn, int *keyval,
                      void *extra_state)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_KEYVAL_CREATE);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_KEYVAL_CREATE);

    mpi_errno = PMPI_Comm_create_keyval(copy_fn, delete_fn, keyval, extra_state);

    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_KEYVAL_CREATE);
    return mpi_errno;
}
