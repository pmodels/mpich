/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_get_name */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_get_name = PMPI_Win_get_name
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_get_name  MPI_Win_get_name
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_get_name as PMPI_Win_get_name
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Win_get_name(MPI_Win win, char *win_name, int *resultlen) __attribute__((weak,alias("PMPI_Win_get_name")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Win_get_name
#define MPI_Win_get_name PMPI_Win_get_name

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Win_get_name
#undef FCNAME
#define FCNAME "MPI_Win_get_name"

/*@
   MPI_Win_get_name - Get the print name associated with the MPI RMA window

Input Parameters:
. win - window whose name is to be returned (handle) 

Output Parameters:
+ win_name - the name previously stored on the window, or a empty string if 
  no such name exists (string) 
- resultlen - length of returned name (integer) 

.N ThreadSafeNoUpdate

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_WIN
.N MPI_ERR_OTHER
.N MPI_ERR_ARG
@*/
int MPI_Win_get_name(MPI_Win win, char *win_name, int *resultlen)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WIN_GET_NAME);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_WIN_GET_NAME);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_WIN(win, mpi_errno);
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
            if (mpi_errno) goto fn_fail;
	    /* If win_ptr is not valid, it will be reset to null */

	    MPIR_ERRTEST_ARGNULL(win_name, "win_name", mpi_errno);
	    MPIR_ERRTEST_ARGNULL(resultlen, "resultlen", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    MPIU_Strncpy( win_name, win_ptr->name, MPI_MAX_OBJECT_NAME );
    *resultlen = (int)strlen( win_name );
    
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_WIN_GET_NAME);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_win_get_name", 
	    "**mpi_win_get_name %W %p %p", win, win_name, resultlen);
    }
#   endif
    mpi_errno = MPIR_Err_return_win( win_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
