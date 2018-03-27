/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Keyval_create */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Keyval_create = PMPI_Keyval_create
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Keyval_create  MPI_Keyval_create
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Keyval_create as PMPI_Keyval_create
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Keyval_create(MPI_Copy_function * copy_fn, MPI_Delete_function * delete_fn,
                      int *keyval, void *extra_state)
    __attribute__ ((weak, alias("PMPI_Keyval_create")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Keyval_create
#define MPI_Keyval_create PMPI_Keyval_create

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Keyval_create
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@

MPI_Keyval_create - Greates a new attribute key

Input Parameters:
+ copy_fn - Copy callback function for 'keyval'
. delete_fn - Delete callback function for 'keyval'
- extra_state - Extra state for callback functions

Output Parameters:
. keyval - key value for future access (integer)

Notes:
Key values are global (available for any and all communicators).

There are subtle differences between C and Fortran that require that the
copy_fn be written in the same language that 'MPI_Keyval_create'
is called from.
This should not be a problem for most users; only programmers using both
Fortran and C in the same program need to be sure that they follow this rule.

.N ThreadSafe

.N Deprecated
The replacement for this routine is 'MPI_Comm_create_keyval'.

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_EXHAUSTED
.N MPI_ERR_ARG

.seealso  MPI_Keyval_free, MPI_Comm_create_keyval
@*/
int MPI_Keyval_create(MPI_Copy_function * copy_fn,
                      MPI_Delete_function * delete_fn, int *keyval, void *extra_state)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_KEYVAL_CREATE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_KEYVAL_CREATE);

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_ARGNULL(keyval, "keyval", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Comm_create_keyval_impl(copy_fn, delete_fn, keyval, extra_state);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_KEYVAL_CREATE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_keyval_create", "**mpi_keyval_create %p %p %p %p", copy_fn,
                                 delete_fn, keyval, extra_state);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
