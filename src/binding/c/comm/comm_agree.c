/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPIX_Comm_agree */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_Comm_agree = PMPIX_Comm_agree
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_Comm_agree  MPIX_Comm_agree
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_Comm_agree as PMPIX_Comm_agree
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPIX_Comm_agree(MPI_Comm comm, int *flag)  __attribute__ ((weak, alias("PMPIX_Comm_agree")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_Comm_agree
#define MPIX_Comm_agree PMPIX_Comm_agree

#endif

/*@
   MPIX_Comm_agree - Performs agreement operation on comm

Input Parameters:
. comm - communicator (handle)

Output Parameters:
. flag - new communicator (logical)

Notes:
The purpose of this collective communication is to agree on the integer value flag and on the group of failed processes in comm.

On completion, all non-failed MPI processes have agreed to set the output integer value of flag to the result of a bitwise AND operation over the contributed input values of flag. If comm is an intercommunicator, the value of flagis a bitwise AND operation over the values contributed by the remote group.

When an MPI process fails before contributing to the operation, the flag is computed ignoring its contribution, and MPIX_Comm_agree raises an error of class MPI_ERR_PROC_FAILED. However, if all MPI processes have acknowledged this failure priorto the call to MPIX_Comm_agree, using MPIX_Comm_failure_ack, the error related to this failure is not raised. When an error of class MPI_ERR_PROC_FAILED is raised, it is consistently raised at allMPI processes, in both the local and remote groups (if applicable).

After MPIX_Comm_agree raised an error of class MPI_ERR_PROC_FAILED, a subse-quent call to MPIX_Comm_failure_ack on comm acknowledges the failure of every MPI process that didn't contribute to the computation offlag.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS

.N MPI_ERR_ARG
.N MPI_ERR_COMM
@*/

int MPIX_Comm_agree(MPI_Comm comm, int *flag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPIX_COMM_AGREE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPIX_COMM_AGREE);

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
            MPIR_ERRTEST_ARGNULL(flag, "flag", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */
    mpi_errno = MPIR_Comm_agree_impl(comm_ptr, flag);
    if (mpi_errno) {
        goto fn_fail;
    }
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPIX_COMM_AGREE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLINE-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                     "**mpix_comm_agree", "**mpix_comm_agree %C %p", comm, flag);
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
