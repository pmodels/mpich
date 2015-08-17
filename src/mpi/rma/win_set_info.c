/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_set_info */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_set_info = PMPI_Win_set_info
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_set_info  MPI_Win_set_info
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_set_info as PMPI_Win_set_info
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Win_set_info(MPI_Win win, MPI_Info info) __attribute__((weak,alias("PMPI_Win_set_info")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Win_set_info
#define MPI_Win_set_info PMPI_Win_set_info

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Win_set_info

/*@
MPI_Win_set_info - Set new values for the hints of the window associated with
win.


The call is collective on the group of win. The info object may be different on
each process, but any info entries that an implementation requires to be the
same on all processes must appear with the same value in each process'' info
object.

Input Parameters:
+ win - window object (handle)
- info - info argument (handle)

Notes:

Some info items that an implementation can use when it creates a window cannot
easily be changed once the window has been created. Thus, an implementation may
ignore hints issued in this call that it would have accepted in a creation
call.

.N ThreadSafe
.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
.N MPI_ERR_WIN
.N MPI_ERR_INFO
.N MPI_ERR_OTHER

.seealso: MPI_Win_get_info
@*/
int MPI_Win_set_info(MPI_Win win, MPI_Info info)
{
    static const char FCNAME[] = "MPI_Win_set_info";
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;
    MPID_Info *info_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WIN_SET_INFO);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_RMA_FUNC_ENTER(MPID_STATE_MPI_WIN_SET_INFO);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INFO(info, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPID_Win_get_ptr( win, win_ptr );
    MPID_Info_get_ptr( info, info_ptr );

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate pointers */
            MPID_Win_valid_ptr( win_ptr, mpi_errno );
            if (mpi_errno) goto fn_fail;

            MPID_Info_valid_ptr( info_ptr, mpi_errno );
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPID_Win_set_info(win_ptr, info_ptr);

    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_WIN_SET_INFO);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_win_set_info", "**mpi_win_set_info %W %I", win, info);
    }
#   endif
    mpi_errno = MPIR_Err_return_win( win_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
