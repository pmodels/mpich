/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPIX_Comm_revoke */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_Comm_revoke = PMPIX_Comm_revoke
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_Comm_revoke  MPIX_Comm_revoke
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_Comm_revoke as PMPIX_Comm_revoke
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPIX_Comm_revoke(MPI_Comm comm)  __attribute__ ((weak, alias("PMPIX_Comm_revoke")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_Comm_revoke
#define MPIX_Comm_revoke PMPIX_Comm_revoke

#endif

/*@
   MPIX_Comm_revoke - Prevent a communicator from being used in the future

Input Parameters:
. comm - communicator to revoke (handle)

Notes:
This function notifies all MPI processes in the groups (local and remote) associated with the communicator comm that this communicator is revoked. The revocation of a communicator by any MPIprocess completes non-local MPI operations on comm at all MPI processes by raising an error of class MPI_ERR_REVOKED (with the exception of MPIX_Comm_shrink, MPIX_Comm_agree). This function is not collective and therefore does not have a matching call on remote MPI processes. All non-failed MPIprocesses belonging to comm will be notified of the revocation despite failures.

A communicator is revoked at a given MPI process either when MPIX_Comm_revoke is locally called on it, or when any MPI operation on comm raises an error of class MPI_ERR_REVOKED at that process. Once a communicator has been revoked at an MPI process, all subsequent non-local operations on that communicator (with thesame exceptions as above), are considered local and must complete by raising an error of class MPI_ERR_REVOKED at that MPI process.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS

.N MPI_ERR_COMM
@*/

int MPIX_Comm_revoke(MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPIX_COMM_REVOKE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPIX_COMM_REVOKE);

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
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */
    mpi_errno = MPID_Comm_revoke(comm_ptr, 0);
    if (mpi_errno) {
        goto fn_fail;
    }
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPIX_COMM_REVOKE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLINE-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                     "**mpix_comm_revoke", "**mpix_comm_revoke %C", comm);
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
