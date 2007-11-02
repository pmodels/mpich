/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_call_errhandler */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_call_errhandler = PMPI_Win_call_errhandler
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_call_errhandler  MPI_Win_call_errhandler
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_call_errhandler as PMPI_Win_call_errhandler
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Win_call_errhandler
#define MPI_Win_call_errhandler PMPI_Win_call_errhandler

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Win_call_errhandler
#undef FCNAME
#define FCNAME "MPI_Win_call_errhander"

/*@
   MPI_Win_call_errhandler - Call the error handler installed on a 
   window object

   Input Parameters:
+ win - window with error handler (handle) 
- errorcode - error code (integer) 

 Note:
 Assuming the input parameters are valid, when the error handler is set to
 MPI_ERRORS_RETURN, this routine will always return MPI_SUCCESS.
 
.N ThreadSafeNoUpdate

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_WIN
@*/
int MPI_Win_call_errhandler(MPI_Win win, int errorcode)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;
    MPIU_THREADPRIV_DECL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WIN_CALL_ERRHANDLER);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_WIN_CALL_ERRHANDLER);

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
    
    if (!win_ptr->errhandler || 
	win_ptr->errhandler->handle == MPI_ERRORS_ARE_FATAL) {
	mpi_errno = MPIR_Err_return_win( win_ptr, "MPI_Win_call_errhandler", errorcode );
	goto fn_exit;
    }

    if (win_ptr->errhandler->handle == MPI_ERRORS_RETURN) {
	/* MPI_ERRORS_RETURN should always return MPI_SUCCESS */
	goto fn_exit;
    }

    /* The user error handler may make calls to MPI routines, so the nesting
     * counter must be incremented before the handler is called */
    MPIR_Nest_incr();
    
    switch (win_ptr->errhandler->language) {
    case MPID_LANG_C:
#ifdef HAVE_CXX_BINDING
    case MPID_LANG_CXX:
#endif
	(*win_ptr->errhandler->errfn.C_Win_Handler_function)( 
	    &win_ptr->handle, &errorcode );
	break;
#ifdef HAVE_FORTRAN_BINDING
    case MPID_LANG_FORTRAN90:
    case MPID_LANG_FORTRAN:
	(*win_ptr->errhandler->errfn.F77_Handler_function)( 
	    (MPI_Fint *)&win_ptr->handle, &errorcode );
	break;
#endif
    }
    
    MPIR_Nest_decr();
    
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_WIN_CALL_ERRHANDLER);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_win_call_errhandler", 
	    "**mpi_win_call_errhandler %W %d", win, errorcode);
    }
    mpi_errno = MPIR_Err_return_win(win_ptr, FCNAME, mpi_errno);
    goto fn_exit;
#   endif
    /* --END ERROR HANDLING-- */
}
