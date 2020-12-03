/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpicomm.h"
#include <stdint.h>

/* -- Begin Profiling Symbol Block for routine MPIX_Comm_agree */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_Comm_agree = PMPIX_Comm_agree
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_Comm_agree  MPIX_Comm_agree
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_Comm_agree as PMPIX_Comm_agree
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPIX_Comm_agree(MPI_Comm comm, int *flag) __attribute__ ((weak, alias("PMPIX_Comm_agree")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_Comm_agree
#define MPIX_Comm_agree PMPIX_Comm_agree
#endif /* !defined(MPICH_MPI_FROM_PMPI) */

/*@
MPIX_Comm_agree - Performs agreement operation on comm

Input Parameters:
. comm - communicator (handle)

Output Parameters:
. newcomm - new communicator (handle)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
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

    /* Validate parameters, and convert MPI object handles to object pointers */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;

        MPIR_Comm_get_ptr(comm, comm_ptr);

        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, TRUE);
            if (mpi_errno)
                goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#else
    {
        MPIR_Comm_get_ptr(comm, comm_ptr);
    }
#endif

    /* ... body of routine ... */
    mpi_errno = MPIR_Comm_agree(comm_ptr, flag);
    MPIR_ERR_CHECK(mpi_errno);

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPIX_COMM_AGREE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                 MPI_ERR_OTHER, "**mpix_comm_agree", "**mpix_comm_agree %C %p",
                                 comm);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
