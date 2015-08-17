/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_free */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_free = PMPI_Win_free
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_free  MPI_Win_free
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_free as PMPI_Win_free
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Win_free(MPI_Win *win) __attribute__((weak,alias("PMPI_Win_free")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Win_free
#define MPI_Win_free PMPI_Win_free

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Win_free

/*@
   MPI_Win_free - Free an MPI RMA window

Input Parameters:
. win - window object (handle) 

   Notes:
   If successfully freed, 'win' is set to 'MPI_WIN_NULL'.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_WIN
.N MPI_ERR_OTHER
@*/
int MPI_Win_free(MPI_Win *win)
{
    static const char FCNAME[] = "MPI_Win_free";
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WIN_FREE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_RMA_FUNC_ENTER(MPID_STATE_MPI_WIN_FREE);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_WIN(*win, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif
    
    /* Convert MPI object handles to object pointers */
    MPID_Win_get_ptr( *win, win_ptr );

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate win_ptr */
            MPID_Win_valid_ptr( win_ptr, mpi_errno );
            if (mpi_errno) goto fn_fail;

            /* TODO: Check for unterminated passive target epoch */

            /* TODO: check for unterminated active mode epoch */
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    if (MPIR_Process.attr_free && win_ptr->attributes)
    {
	mpi_errno = MPIR_Process.attr_free( win_ptr->handle, 
					    &win_ptr->attributes );
    }
    /*
     * If the user attribute free function returns an error, 
     * then do not free the window
     */
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* We need to release the error handler */
    if (win_ptr->errhandler && 
	! (HANDLE_GET_KIND(win_ptr->errhandler->handle) == 
	   HANDLE_KIND_BUILTIN) ) {
	int in_use;
	MPIR_Errhandler_release_ref( win_ptr->errhandler,&in_use);
	if (!in_use) {
	    MPIU_Handle_obj_free( &MPID_Errhandler_mem, win_ptr->errhandler );
	}
    }
    
    mpi_errno = MPID_Win_free(&win_ptr);
    if (mpi_errno) goto fn_fail;
    *win = MPI_WIN_NULL;

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_WIN_FREE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_win_free", "**mpi_win_free %p", win);
    }
#   endif
    mpi_errno = MPIR_Err_return_win( win_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
