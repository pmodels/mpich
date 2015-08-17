/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

/* -- Begin Profiling Symbol Block for routine MPIX_Comm_revoke */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_Comm_revoke = PMPIX_Comm_revoke
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_Comm_revoke  MPIX_Comm_revoke
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_Comm_revoke as PMPIX_Comm_revoke
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPIX_Comm_revoke(MPI_Comm comm) __attribute__((weak,alias("PMPIX_Comm_revoke")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPIX_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_Comm_revoke
#define MPIX_Comm_revoke PMPIX_Comm_revoke

#endif

#undef FUNCNAME
#define FUNCNAME MPIX_Comm_revoke
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
    MPIX_Comm_revoke - Prevent a communicator from being used in the future

Input Parameters:
+ comm - communicator to revoke

Notes:
Asynchronously notifies all MPI processes associated with the communicator 'comm'.
This will be manifest by returning the MPIX_ERR_REVOKED during a subsequent MPI
call.

.N Fortran

.N Errors
.N MPIX_SUCCESS
@*/
int MPIX_Comm_revoke(MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIX_COMM_REVOKE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIX_COMM_REVOKE);

    /* Validate parameters, especially handles needing to be converted */
#    ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif

    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr( comm, comm_ptr );

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno, TRUE );
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif

    /* ... body of routine ... */

    mpi_errno = MPID_Comm_revoke(comm_ptr, 0);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIX_COMM_REVOKE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpix_comm_revoke",
            "**mpix_comm_revoke %C", comm);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
