/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPIX_Comm_failure_get_acked */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_Comm_failure_get_acked = PMPIX_Comm_failure_get_acked
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_Comm_failure_get_acked  MPIX_Comm_failure_get_acked
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_Comm_failure_get_acked as PMPIX_Comm_failure_get_acked
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPIX_Comm_failure_get_acked(MPI_Comm comm, MPI_Group *failedgrp)
     __attribute__ ((weak, alias("PMPIX_Comm_failure_get_acked")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_Comm_failure_get_acked
#define MPIX_Comm_failure_get_acked PMPIX_Comm_failure_get_acked

#endif

/*@
   MPIX_Comm_failure_get_acked - Get the group of acknowledged failures.

Input Parameters:
. comm - communicator (handle)

Output Parameters:
. failedgrp - group of failed processes (handle)

Notes:
This local operation returns the group failedgrp of processes, from the communicatorcomm, that have been locally acknowledged as failed by preceding calls to MPIX_Comm_failure_ack. The failedgrp can be empty, that is, equal to MPI_GROUP_EMPTY.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS

.N MPI_ERR_ARG
.N MPI_ERR_COMM
@*/

int MPIX_Comm_failure_get_acked(MPI_Comm comm, MPI_Group *failedgrp)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPIX_COMM_FAILURE_GET_ACKED);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPIX_COMM_FAILURE_GET_ACKED);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    MPIR_Comm_get_ptr(comm, comm_ptr);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, TRUE);
            if (mpi_errno) {
                goto fn_fail;
            }
            MPIR_ERRTEST_ARGNULL(failedgrp, "failedgrp", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */
    MPIR_Group *failedgrp_ptr = NULL;
    *failedgrp = MPI_GROUP_NULL;
    mpi_errno = MPID_Comm_failure_get_acked(comm_ptr, &failedgrp_ptr);
    if (mpi_errno) {
        goto fn_fail;
    }
    if (failedgrp_ptr) {
        MPIR_OBJ_PUBLISH_HANDLE(*failedgrp, failedgrp_ptr->handle);
    }
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPIX_COMM_FAILURE_GET_ACKED);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLINE-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                     "**mpix_comm_failure_get_acked",
                                     "**mpix_comm_failure_get_acked %C %p", comm, failedgrp);
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
