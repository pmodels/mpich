/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_get_group */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_get_group = PMPI_Win_get_group
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_get_group  MPI_Win_get_group
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_get_group as PMPI_Win_get_group
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Win_get_group(MPI_Win win, MPI_Group *group) __attribute__((weak,alias("PMPI_Win_get_group")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Win_get_group
#define MPI_Win_get_group PMPI_Win_get_group

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Win_get_group

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
.N MPI_ERR_WIN
.N MPI_ERR_ARG
.N MPI_ERR_OTHER
@*/
int MPI_Win_get_group(MPI_Win win, MPI_Group *group)
{
    static const char FCNAME[] = "MPI_Win_get_group";
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;
    MPID_Comm *win_comm_ptr;
    MPID_Group *group_ptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WIN_GET_GROUP);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_RMA_FUNC_ENTER(MPID_STATE_MPI_WIN_GET_GROUP);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_WIN(win, mpi_errno);
            MPIR_ERRTEST_ARGNULL(group, "group", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif
    
    /* Convert MPI object handles to object pointers */
    MPID_Win_get_ptr( win, win_ptr );
    
    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate win_ptr */
            MPID_Win_valid_ptr( win_ptr, mpi_errno );
	    /* If win_ptr is not valid, it will be reset to null */

            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    win_comm_ptr = win_ptr->comm_ptr;

    mpi_errno = MPIR_Comm_group_impl(win_comm_ptr, &group_ptr);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
    *group = group_ptr->handle;
    
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_WIN_GET_GROUP);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_win_get_group", 
	    "**mpi_win_get_group %W %p", win, group);
    }
#   endif
    mpi_errno = MPIR_Err_return_win( win_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
