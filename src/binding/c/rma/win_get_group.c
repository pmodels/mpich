/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_get_group */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_get_group = PMPI_Win_get_group
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_get_group  MPI_Win_get_group
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_get_group as PMPI_Win_get_group
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Win_get_group(MPI_Win win, MPI_Group *group)
     __attribute__ ((weak, alias("PMPI_Win_get_group")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Win_get_group
#define MPI_Win_get_group PMPI_Win_get_group

#endif

/*@
   MPI_Win_get_group - Get the MPI Group of the window object

Input Parameters:
. win - window object (handle)

Output Parameters:
. group - group of processes which share access to the window (handle)

Notes:
The group is a duplicate of the group from the communicator used to
create the MPI window, and should be freed with 'MPI_Group_free' when
it is no longer needed.  This group can be used to form the group of
neighbors for the routines 'MPI_Win_post' and 'MPI_Win_start'.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS

.N MPI_ERR_ARG
.N MPI_ERR_WIN
@*/

int MPI_Win_get_group(MPI_Win win, MPI_Group *group)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Win *win_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_WIN_GET_GROUP);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_WIN_GET_GROUP);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_WIN(win, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    MPIR_Win_get_ptr(win, win_ptr);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_Win_valid_ptr(win_ptr, mpi_errno);
            if (mpi_errno) {
                goto fn_fail;
            }
            MPIR_ERRTEST_ARGNULL(group, "group", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */
    MPIR_Group *group_ptr = NULL;
    MPIR_Comm *win_comm_ptr = win_ptr->comm_ptr;
    mpi_errno = MPIR_Comm_group_impl(win_comm_ptr, &group_ptr);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;
    *group = group_ptr->handle;
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_WIN_GET_GROUP);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLINE-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                     "**mpi_win_get_group", "**mpi_win_get_group %W %p", win, group);
#endif
    mpi_errno = MPIR_Err_return_win(win_ptr, __func__, mpi_errno);
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
