/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Test_cancelled */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Test_cancelled = PMPI_Test_cancelled
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Test_cancelled  MPI_Test_cancelled
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Test_cancelled as PMPI_Test_cancelled
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Test_cancelled(const MPI_Status *status, int *flag) __attribute__((weak,alias("PMPI_Test_cancelled")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Test_cancelled
#define MPI_Test_cancelled PMPI_Test_cancelled

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Test_cancelled
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
  MPI_Test_cancelled - Tests to see if a request was cancelled

Input Parameters:
. status - status object (Status) 

Output Parameters:
. flag - true if the request was cancelled, false otherwise (logical) 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
@*/
int MPI_Test_cancelled(const MPI_Status *status, int *flag)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TEST_CANCELLED);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_MPI_PT2PT_FUNC_ENTER(MPID_STATE_MPI_TEST_CANCELLED);
    
    /* Validate parameters if error checking is enabled */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_ARGNULL( status, "status", mpi_errno );
	    MPIR_ERRTEST_ARGNULL( flag, "flag", mpi_errno );
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    MPIR_Test_cancelled_impl(status, flag);
    
    /* ... end of body of routine ... */
    
#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_TEST_CANCELLED);
    return mpi_errno;
    
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_test_cancelled",
	    "**mpi_test_cancelled %p %p", status, flag);
    }
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
#   endif
    /* --END ERROR HANDLING-- */
}

