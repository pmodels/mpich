/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Is_thread_main */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Is_thread_main = PMPI_Is_thread_main
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Is_thread_main  MPI_Is_thread_main
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Is_thread_main as PMPI_Is_thread_main
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Is_thread_main
#define MPI_Is_thread_main PMPI_Is_thread_main
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Is_thread_main

/*@
   MPI_Is_thread_main - Returns a flag indicating whether this thread called 
                        'MPI_Init' or 'MPI_Init_thread'

   Output Parameter:
. flag - Flag is true if 'MPI_Init' or 'MPI_Init_thread' has been called by 
         this thread and false otherwise.  (logical)

.N SignalSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Is_thread_main( int *flag )
{
#ifdef HAVE_ERROR_CHECKING
    static const char FCNAME[] = "MPI_Is_thread_main";
#endif
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_IS_THREAD_MAIN);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_ARGNULL(flag,"flag",mpi_errno);
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_IS_THREAD_MAIN);
    
    /* ... body of routine ...  */
#   if MPICH_THREAD_LEVEL <= MPI_THREAD_FUNNELED || ! defined(MPICH_IS_THREADED)
    {
	*flag = TRUE;
    }
#   else
    {
	MPID_Thread_id_t my_thread_id;

	MPID_Thread_self(&my_thread_id);
	MPID_Thread_same(&MPIR_ThreadInfo.master_thread, &my_thread_id, flag);
    }
#   endif
    /* ... end of body of routine ... */

#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_IS_THREAD_MAIN);
    return mpi_errno;
    
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, 
	    MPI_ERR_OTHER, "**mpi_is_thread_main",
	    "**mpi_is_thread_main %p", flag);
    }
    mpi_errno = MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    goto fn_exit;
#   endif
    /* --END ERROR HANDLING-- */
}
