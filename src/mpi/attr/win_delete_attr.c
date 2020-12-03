/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_delete_attr */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_delete_attr = PMPI_Win_delete_attr
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_delete_attr  MPI_Win_delete_attr
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_delete_attr as PMPI_Win_delete_attr
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Win_delete_attr(MPI_Win win, int win_keyval)
    __attribute__ ((weak, alias("PMPI_Win_delete_attr")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Win_delete_attr
#define MPI_Win_delete_attr PMPI_Win_delete_attr

#endif

/*@
   MPI_Win_delete_attr - Deletes an attribute value associated with a key on
   a datatype

Input Parameters:
+ win - window from which the attribute is deleted (handle)
- win_keyval - key value (integer)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_WIN
.N MPI_ERR_KEYVAL
.N MPI_ERR_OTHER
@*/
int MPI_Win_delete_attr(MPI_Win win, int win_keyval)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Win *win_ptr = NULL;
    MPII_Keyval *keyval_ptr = 0;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_WIN_DELETE_ATTR);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    /* The thread lock prevents a valid attr delete on the same window
     * but in a different thread from causing problems */
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_WIN_DELETE_ATTR);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_WIN(win, mpi_errno);
            MPIR_ERRTEST_KEYVAL(win_keyval, MPIR_WIN, "window", mpi_errno);
            MPIR_ERRTEST_KEYVAL_PERM(win_keyval, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif

    /* Convert MPI object handles to object pointers */
    MPIR_Win_get_ptr(win, win_ptr);
    MPII_Keyval_get_ptr(win_keyval, keyval_ptr);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate win_ptr */
            MPIR_Win_valid_ptr(win_ptr, mpi_errno);
            /* If win_ptr is not valid, it will be reset to null */
            /* Validate keyval_ptr */
            MPII_Keyval_valid_ptr(keyval_ptr, mpi_errno);
            if (mpi_errno)
                goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Win_delete_attr_impl(win_ptr, keyval_ptr);
    if (mpi_errno != MPI_SUCCESS) {
        goto fn_fail;
    }

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_WIN_DELETE_ATTR);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_win_delete_attr", "**mpi_win_delete_attr %W %K", win,
                                 win_keyval);
    }
#endif
    mpi_errno = MPIR_Err_return_win(win_ptr, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
