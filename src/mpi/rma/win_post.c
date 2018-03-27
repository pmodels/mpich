/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_post */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_post = PMPI_Win_post
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_post  MPI_Win_post
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_post as PMPI_Win_post
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Win_post(MPI_Group group, int assert, MPI_Win win)
    __attribute__ ((weak, alias("PMPI_Win_post")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Win_post
#define MPI_Win_post PMPI_Win_post

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Win_post
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
   MPI_Win_post - Start an RMA exposure epoch

Input Parameters:
+ group - group of origin processes (handle)
. assert - Used to optimize this call; zero may be used as a default.
  See notes. (integer)
- win - window object (handle)

   Notes:
   The 'assert' argument is used to indicate special conditions for the
   fence that an implementation may use to optimize the 'MPI_Win_post'
   operation.  The value zero is always correct.  Other assertion values
   may be or''ed together.  Assertions that are valid for 'MPI_Win_post' are\:

+ MPI_MODE_NOCHECK - the matching calls to 'MPI_WIN_START' have not yet
  occurred on any origin processes when the call to 'MPI_WIN_POST' is made.
  The nocheck option can be specified by a post call if and only if it is
  specified by each matching start call.
. MPI_MODE_NOSTORE - the local window was not updated by local stores (or
  local get or receive calls) since last synchronization. This may avoid
  the need for cache synchronization at the post call.
- MPI_MODE_NOPUT - the local window will not be updated by put or accumulate
  calls after the post call, until the ensuing (wait) synchronization. This
  may avoid the need for cache synchronization at the wait call.

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Win_post(MPI_Group group, int assert, MPI_Win win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Win *win_ptr = NULL;
    MPIR_Group *group_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_WIN_POST);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_RMA_ENTER(MPID_STATE_MPI_WIN_POST);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_WIN(win, mpi_errno);
            MPIR_ERRTEST_GROUP(group, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif

    /* Convert MPI object handles to object pointers */
    MPIR_Win_get_ptr(win, win_ptr);
    MPIR_Group_get_ptr(group, group_ptr);

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate win_ptr */
            MPIR_Win_valid_ptr(win_ptr, mpi_errno);

            MPIR_Group_valid_ptr(group_ptr, mpi_errno);

            /* TODO: Validate assert argument */
            /* TODO: Validate that window is not in passive mode */

            if (mpi_errno)
                goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPID_Win_post(group_ptr, assert, win_ptr);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_RMA_EXIT(MPID_STATE_MPI_WIN_POST);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_win_post", "**mpi_win_post %G %A %W", group, assert, win);
    }
#endif
    mpi_errno = MPIR_Err_return_win(win_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
