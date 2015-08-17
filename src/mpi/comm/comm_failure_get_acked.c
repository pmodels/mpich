/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpicomm.h"

/* -- Begin Profiling Symbol Block for routine MPIX_Comm_get_acked */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_Comm_failure_get_acked = PMPIX_Comm_failure_get_acked
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_Comm_failure_get_acked  MPIX_Comm_failure_get_acked
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_Comm_failure_get_acked as PMPIX_Comm_failure_get_acked
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPIX_Comm_failure_get_acked( MPI_Comm comm, MPI_Group *failedgrp ) __attribute__((weak,alias("PMPIX_Comm_failure_get_acked")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_Comm_failure_get_acked
#define MPIX_Comm_failure_get_acked PMPIX_Comm_failure_get_acked

#endif

#undef FUNCNAME
#define FUNCNAME MPIX_Comm_failure_get_acked
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@

MPIX_Comm_failure_get_acked - Get the group of acknowledged failures.

Input Parameters:
. comm - Communicator (handle)

Output Parameters:
. failed_group - Group (handle)

Notes:
.N COMMNULL

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
@*/
int MPIX_Comm_failure_get_acked( MPI_Comm comm, MPI_Group *failedgrp )
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_Group *group_ptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIX_COMM_FAILURE_GET_ACKED);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIX_COMM_FAILURE_GET_ACKED);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr(comm, comm_ptr);

    /* Validate parameters and objects(post conversion */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno, TRUE );
            /* If comm_ptr is not valid, it will be reset to null */
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */

    mpi_errno = MPID_Comm_failure_get_acked(comm_ptr, &group_ptr);
    if (mpi_errno) goto fn_fail;
    *failedgrp = group_ptr->handle;
    /* ... end of body of routine ... */

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIX_COMM_FAILURE_GET_ACKED);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
                mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpix_comm_failure_get_acked",
                "**mpix_comm_failure_get_acked %C %p", comm, failedgrp);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
