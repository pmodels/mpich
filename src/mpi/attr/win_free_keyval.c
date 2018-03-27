/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "attr.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_free_keyval */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_free_keyval = PMPI_Win_free_keyval
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_free_keyval  MPI_Win_free_keyval
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_free_keyval as PMPI_Win_free_keyval
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Win_free_keyval(int *win_keyval) __attribute__ ((weak, alias("PMPI_Win_free_keyval")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Win_free_keyval
#define MPI_Win_free_keyval PMPI_Win_free_keyval

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Win_free_keyval
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
   MPI_Win_free_keyval - Frees an attribute key for MPI RMA windows

Input Parameters:
. win_keyval - key value (integer)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_WIN
.N MPI_ERR_OTHER
.N MPI_ERR_KEYVAL
@*/
int MPI_Win_free_keyval(int *win_keyval)
{
    int mpi_errno = MPI_SUCCESS;
    MPII_Keyval *keyval_ptr = NULL;
    int in_use;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_WIN_FREE_KEYVAL);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_WIN_FREE_KEYVAL);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_ARGNULL(*win_keyval, "win_keyval", mpi_errno);
            MPIR_ERRTEST_KEYVAL(*win_keyval, MPIR_WIN, "window", mpi_errno);
            MPIR_ERRTEST_KEYVAL_PERM(*win_keyval, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif

    /* Convert MPI object handles to object pointers */
    MPII_Keyval_get_ptr(*win_keyval, keyval_ptr);

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPII_Keyval_valid_ptr(keyval_ptr, mpi_errno);
            if (mpi_errno)
                goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    if (!keyval_ptr->was_freed) {
        keyval_ptr->was_freed = 1;
        MPII_Keyval_release_ref(keyval_ptr, &in_use);
        if (!in_use) {
            MPIR_Handle_obj_free(&MPII_Keyval_mem, keyval_ptr);
        }
    }
    *win_keyval = MPI_KEYVAL_INVALID;

    /* ... end of body of routine ... */

#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_WIN_FREE_KEYVAL);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_win_free_keyval", "**mpi_win_free_keyval %p", win_keyval);
    }
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
#endif
    /* --END ERROR HANDLING-- */
}
