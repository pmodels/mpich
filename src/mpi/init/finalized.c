/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Finalized */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Finalized = PMPI_Finalized
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Finalized  MPI_Finalized
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Finalized as PMPI_Finalized
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Finalized(int *flag) __attribute__((weak,alias("PMPI_Finalized")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Finalized
#define MPI_Finalized PMPI_Finalized
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Finalized

/*@
   MPI_Finalized - Indicates whether 'MPI_Finalize' has been called.

Output Parameters:
. flag - Flag is true if 'MPI_Finalize' has been called and false otherwise. 
     (logical)

.N SignalSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Finalized( int *flag )
{
#ifdef HAVE_ERROR_CHECKING
    static const char FCNAME[] = "MPI_Finalized";
#endif
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_FINALIZED);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_FINALIZED);

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    /* Should check that flag is not null */
	    if (flag == NULL)
	    {
		mpi_errno = MPI_ERR_ARG;
		goto fn_fail;
	    }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    *flag = (OPA_load_int(&MPIR_Process.mpich_state) >= MPICH_POST_FINALIZED);
    
    /* ... end of body of routine ... */

#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_FINALIZED);
    return mpi_errno;
    
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
  fn_fail:
    if (OPA_load_int(&MPIR_Process.mpich_state) == MPICH_IN_INIT ||
        OPA_load_int(&MPIR_Process.mpich_state) == MPICH_POST_INIT)
    {
	{
	    mpi_errno = MPIR_Err_create_code(
		mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, 
		MPI_ERR_OTHER, "**mpi_finalized",
		"**mpi_finalized %p", flag);
	}
	
	mpi_errno = MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    }
    goto fn_exit;
#   endif
    /* --END ERROR HANDLING-- */
}
