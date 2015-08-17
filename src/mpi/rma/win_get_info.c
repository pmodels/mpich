/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_get_info */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_get_info = PMPI_Win_get_info
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_get_info  MPI_Win_get_info
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_get_info as PMPI_Win_get_info
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Win_get_info(MPI_Win win, MPI_Info *info_used) __attribute__((weak,alias("PMPI_Win_get_info")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Win_get_info
#define MPI_Win_get_info PMPI_Win_get_info

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Win_get_info

/*@
MPI_Win_get_info - Returns a new info object containing the hints of the window
associated with win.


The current setting of all hints actually used by the system related to this
window is returned in info_used. If no such hints exist, a handle to a newly
created info object is returned that contains no key/value pair. The user is
responsible for freeing info_used via 'MPI_Info_free'.

Input Parameters:
. win - window object (handle)

Output Parameters:
. info_used - new info argument (handle)

Notes:

The info object returned in info_used will contain all hints currently active
for this window. This set of hints may be greater or smaller than the set of
hints specified when the window was created, as the system may not recognize
some hints set by the user, and may recognize other hints that the user has not
set.

.N ThreadSafe
.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
.N MPI_ERR_WIN
.N MPI_ERR_INFO
.N MPI_ERR_OTHER

.seealso: MPI_Win_set_info
@*/
int MPI_Win_get_info(MPI_Win win, MPI_Info *info_used)
{
    static const char FCNAME[] = "MPI_Win_get_info";
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;
    MPID_Info *info_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WIN_GET_INFO);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_RMA_FUNC_ENTER(MPID_STATE_MPI_WIN_GET_INFO);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_ARGNULL(info_used, "info", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPID_Win_get_ptr( win, win_ptr );

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate pointers */
            MPID_Win_valid_ptr( win_ptr, mpi_errno );
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPID_Win_get_info(win_ptr, &info_ptr);

    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    *info_used = info_ptr->handle;

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_WIN_GET_INFO);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_win_get_info", "**mpi_win_get_info %W %p", win, info_used);
    }
#   endif
    mpi_errno = MPIR_Err_return_win( win_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
