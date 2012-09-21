/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "rma.h"

/* -- Begin Profiling Symbol Block for routine MPIX_Win_create_dynamic */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_Win_create_dynamic = PMPIX_Win_create_dynamic
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_Win_create_dynamic  MPIX_Win_create_dynamic
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_Win_create_dynamic as PMPIX_Win_create_dynamic
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_Win_create_dynamic
#define MPIX_Win_create_dynamic PMPIX_Win_create_dynamic

#endif

#undef FUNCNAME
#define FUNCNAME MPIX_Win_create_dynamic

/*@
   MPIX_Win_create_dynamic - Create an MPI Window object for one-sided communication

   Input Parameters:
+ info - info argument (handle) 
- comm - communicator (handle) 

  Output Parameter:
. win - window object returned by the call (handle) 

.N ThreadSafe
.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
.N MPI_ERR_COMM
.N MPI_ERR_INFO
.N MPI_ERR_OTHER
.N MPI_ERR_SIZE
@*/
int MPIX_Win_create_dynamic(MPI_Info info, MPI_Comm comm, MPI_Win *win)
{
    static const char FCNAME[] = "MPIX_Win_create_dynamic";
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;
    MPID_Comm *comm_ptr = NULL;
    MPID_Info *info_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIX_WIN_CREATE_DYNAMIC);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_RMA_FUNC_ENTER(MPID_STATE_MPIX_WIN_CREATE_DYNAMIC);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);
            MPIR_ERRTEST_INFO_OR_NULL(info, mpi_errno);
            MPIR_ERRTEST_ARGNULL(win, "win", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr( comm, comm_ptr );
    MPID_Info_get_ptr( info, info_ptr );

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate pointers */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    mpi_errno = MPID_Win_create_dynamic(info_ptr, comm_ptr, &win_ptr);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* Initialize a few fields that have specific defaults */
    win_ptr->name[0]    = 0;
    win_ptr->errhandler = 0;
    win_ptr->lockRank   = MPID_WIN_STATE_UNLOCKED;

    /* return the handle of the window object to the user */
    MPIU_OBJ_PUBLISH_HANDLE(*win, win_ptr->handle);

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPIX_WIN_CREATE_DYNAMIC);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpix_win_create_dynamic", 
            "**mpix_win_create_dynamic %I %C %p", info, comm, win);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
