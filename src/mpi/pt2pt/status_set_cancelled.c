/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Status_set_cancelled */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Status_set_cancelled = PMPI_Status_set_cancelled
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Status_set_cancelled  MPI_Status_set_cancelled
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Status_set_cancelled as PMPI_Status_set_cancelled
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Status_set_cancelled(MPI_Status *status, int flag) __attribute__((weak,alias("PMPI_Status_set_cancelled")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Status_set_cancelled
#define MPI_Status_set_cancelled PMPI_Status_set_cancelled

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Status_set_cancelled

/*@
   MPI_Status_set_cancelled - Sets the cancelled state associated with a 
   Status object

Input Parameters:
+  status - status to associate cancel flag with (Status)
-  flag - if true indicates request was cancelled (logical)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
@*/
int MPI_Status_set_cancelled(MPI_Status *status, int flag)
{
#ifdef HAVE_ERROR_CHECKING
    static const char FCNAME[] = "MPI_Status_set_cancelled";
#endif
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_STATUS_SET_CANCELLED);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_STATUS_SET_CANCELLED);

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_ARGNULL( status, "status", mpi_errno );
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    MPIR_STATUS_SET_CANCEL_BIT(*status, flag ? TRUE : FALSE);

    /* ... end of body of routine ... */
    
#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_STATUS_SET_CANCELLED);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_status_set_cancelled",
	    "**mpi_status_set_cancelled %p %d", status, flag);
    }
    mpi_errno = MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    goto fn_exit;
#   endif
    /* --END ERROR HANDLING-- */
}

