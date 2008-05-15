/* -*- Mode: C; c-basic-offset:4 ; -*- */
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

   Input Parameter:
. win - window object (handle) 

   Output Parameter:
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
    MPIU_THREADPRIV_DECL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WIN_GET_GROUP);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_SINGLE_CS_ENTER("rma");
    MPID_MPI_RMA_FUNC_ENTER(MPID_STATE_MPI_WIN_GET_GROUP);

    MPIU_THREADPRIV_GET;

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_WIN(win, mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
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
    
    MPIR_Nest_incr();
    mpi_errno = NMPI_Comm_group(win_ptr->comm, group);
    MPIR_Nest_decr();
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_WIN_GET_GROUP);
    MPIU_THREAD_SINGLE_CS_EXIT("rma");
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
