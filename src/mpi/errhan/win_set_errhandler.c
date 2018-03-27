/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_set_errhandler */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_set_errhandler = PMPI_Win_set_errhandler
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_set_errhandler  MPI_Win_set_errhandler
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_set_errhandler as PMPI_Win_set_errhandler
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Win_set_errhandler(MPI_Win win, MPI_Errhandler errhandler)
    __attribute__ ((weak, alias("PMPI_Win_set_errhandler")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Win_set_errhandler
#define MPI_Win_set_errhandler PMPI_Win_set_errhandler

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Win_set_errhandler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
   MPI_Win_set_errhandler - Set window error handler

Input Parameters:
+ win - window (handle)
- errhandler - new error handler for window (handle)

.N ThreadSafeNoUpdate

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_WIN
@*/
int MPI_Win_set_errhandler(MPI_Win win, MPI_Errhandler errhandler)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Win *win_ptr = NULL;
    int in_use;
    MPIR_Errhandler *errhan_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_WIN_SET_ERRHANDLER);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_WIN_SET_ERRHANDLER);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_WIN(win, mpi_errno);
            MPIR_ERRTEST_ERRHANDLER(errhandler, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif

    /* Convert MPI object handles to object pointers */
    MPIR_Win_get_ptr(win, win_ptr);
    MPIR_Errhandler_get_ptr(errhandler, errhan_ptr);

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate win_ptr */
            MPIR_Win_valid_ptr(win_ptr, mpi_errno);
            /* If win_ptr is not value, it will be reset to null */

            if (HANDLE_GET_KIND(errhandler) != HANDLE_KIND_BUILTIN) {
                MPIR_Errhandler_valid_ptr(errhan_ptr, mpi_errno);
                /* Also check for a valid errhandler kind */
                if (!mpi_errno) {
                    if (errhan_ptr->kind != MPIR_WIN) {
                        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME,
                                                         __LINE__, MPI_ERR_ARG, "**errhandnotwin",
                                                         NULL);
                    }
                }
            }
            if (mpi_errno)
                goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_POBJ_WIN_MUTEX(win_ptr));

    if (win_ptr->errhandler != NULL) {
        MPIR_Errhandler_release_ref(win_ptr->errhandler, &in_use);
        if (!in_use) {
            MPIR_Errhandler_free(win_ptr->errhandler);
        }
    }

    MPIR_Errhandler_add_ref(errhan_ptr);
    win_ptr->errhandler = errhan_ptr;

    MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_WIN_MUTEX(win_ptr));

    /* ... end of body of routine ... */

#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_WIN_SET_ERRHANDLER);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_win_set_errhandler", "**mpi_win_set_errhandler %W %E", win,
                                 errhandler);
    }
    mpi_errno = MPIR_Err_return_win(win_ptr, FCNAME, mpi_errno);
    goto fn_exit;
#endif
    /* --END ERROR HANDLING-- */
}
