/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
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
int MPIX_Comm_agree(MPI_Comm comm, int *flag) __attribute__((weak,alias("PMPIX_Comm_agree")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_Comm_agree
#define MPIX_Comm_agree PMPIX_Comm_agree
#endif

#undef FUNCNAME
#define FUNCNAME MPIR_Comm_agree
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Comm_agree(MPID_Comm *comm_ptr, int *flag)
{
    int mpi_errno = MPI_SUCCESS, mpi_errno_tmp = MPI_SUCCESS;
    MPID_Group *comm_grp, *failed_grp, *new_group_ptr, *global_failed;
    int result, success = 1;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    int values[2];

    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_COMM_AGREE);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_COMM_AGREE);

    MPIR_Comm_group_impl(comm_ptr, &comm_grp);

    /* Get the locally known (not acknowledged) group of failed procs */
    mpi_errno = MPID_Comm_failure_get_acked(comm_ptr, &failed_grp);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* First decide on the group of failed procs. */
    mpi_errno = MPID_Comm_get_all_failed_procs(comm_ptr, &global_failed, MPIR_AGREE_TAG);
    if (mpi_errno) errflag = MPIR_ERR_PROC_FAILED;

    mpi_errno = MPIR_Group_compare_impl(failed_grp, global_failed, &result);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* Create a subgroup without the failed procs */
    mpi_errno = MPIR_Group_difference_impl(comm_grp, global_failed, &new_group_ptr);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* If that group isn't the same as what we think is failed locally, then
     * mark it as such. */
    if (result == MPI_UNEQUAL || errflag)
        success = 0;

    /* Do an allreduce to decide whether or not anyone thinks the group
     * has changed */
    mpi_errno_tmp = MPIR_Allreduce_group(MPI_IN_PLACE, &success, 1, MPI_INT, MPI_MIN, comm_ptr,
                                         new_group_ptr, MPIR_AGREE_TAG, &errflag);
    if (!success || errflag || mpi_errno_tmp)
        success = 0;

    values[0] = success;
    values[1] = *flag;

    /* Determine both the result of this function (mpi_errno) and the result
     * of flag that will be returned to the user. */
    MPIR_Allreduce_group(MPI_IN_PLACE, values, 2, MPI_INT, MPI_BAND, comm_ptr,
                         new_group_ptr, MPIR_AGREE_TAG, &errflag);
    /* Ignore the result of the operation this time. Everyone will either
     * return a failure because of !success earlier or they will return
     * something useful for flag because of this operation. If there was a new
     * failure in between the first allreduce and the second one, it's ignored
     * here. */

    if (failed_grp != MPID_Group_empty)
        MPIR_Group_release(failed_grp);
    MPIR_Group_release(new_group_ptr);
    MPIR_Group_release(comm_grp);
    if (global_failed != MPID_Group_empty)
        MPIR_Group_release(global_failed);

    success = values[0];
    *flag = values[1];

    if (!success) {
        MPIR_ERR_SET(mpi_errno_tmp, MPIX_ERR_PROC_FAILED, "**mpix_comm_agree");
        MPIR_ERR_ADD(mpi_errno, mpi_errno_tmp);
    }

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_COMM_AGREE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIX_Comm_agree
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPIX_Comm_agree - Performs agreement operation on comm

Input Parameters:
+ comm - communicator (handle)

Output Parameters:
. newcomm - new communicator (handle)

.N Threadsafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM

@*/
int MPIX_Comm_agree(MPI_Comm comm, int *flag)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIX_COMM_AGREE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIX_COMM_AGREE);

    /* Validate parameters, and convert MPI object handles to object pointers */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;

        MPID_Comm_get_ptr( comm, comm_ptr );

        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno, TRUE );
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#else
    {
        MPID_Comm_get_ptr( comm, comm_ptr );
    }
#endif

    /* ... body of routine ... */
    mpi_errno = MPIR_Comm_agree(comm_ptr, flag);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIX_COMM_AGREE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,
                                 MPI_ERR_OTHER, "**mpix_comm_agree",
                                 "**mpix_comm_agree %C", comm);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
