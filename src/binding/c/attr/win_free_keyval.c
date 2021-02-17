/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_free_keyval */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_free_keyval = PMPI_Win_free_keyval
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_free_keyval  MPI_Win_free_keyval
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_free_keyval as PMPI_Win_free_keyval
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Win_free_keyval(int *win_keyval)  __attribute__ ((weak, alias("PMPI_Win_free_keyval")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Win_free_keyval
#define MPI_Win_free_keyval PMPI_Win_free_keyval

#endif

/*@
   MPI_Win_free_keyval - Frees an attribute key for MPI RMA windows

Input/Output Parameters:
. win_keyval - key value (integer)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS

.N MPI_ERR_KEYVAL
@*/

int MPI_Win_free_keyval(int *win_keyval)
{
    int mpi_errno = MPI_SUCCESS;
    MPII_Keyval *win_keyval_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_WIN_FREE_KEYVAL);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_WIN_FREE_KEYVAL);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_ARGNULL(win_keyval, "win_keyval", mpi_errno);
            MPIR_ERRTEST_KEYVAL(*win_keyval, MPIR_WIN, "window", mpi_errno);
            MPIR_ERRTEST_KEYVAL_PERM(*win_keyval, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    MPII_Keyval_get_ptr(*win_keyval, win_keyval_ptr);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPII_Keyval_valid_ptr(win_keyval_ptr, mpi_errno);
            if (mpi_errno) {
                goto fn_fail;
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */
    MPIR_free_keyval(win_keyval_ptr);
    *win_keyval = MPI_KEYVAL_INVALID;
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_WIN_FREE_KEYVAL);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLINE-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                     "**mpi_win_free_keyval", "**mpi_win_free_keyval %p", win_keyval);
#endif
    mpi_errno = MPIR_Err_return_comm(0, __func__, mpi_errno);
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
