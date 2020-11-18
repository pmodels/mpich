/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpicomm.h"

/* -- Begin Profiling Symbol Block for routine MPI_Intercomm_merge */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Intercomm_merge = PMPI_Intercomm_merge
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Intercomm_merge  MPI_Intercomm_merge
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Intercomm_merge as PMPI_Intercomm_merge
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Intercomm_merge(MPI_Comm intercomm, int high, MPI_Comm * newintracomm)
    __attribute__ ((weak, alias("PMPI_Intercomm_merge")));
#endif
/* -- End Profiling Symbol Block */


/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Intercomm_merge
#define MPI_Intercomm_merge PMPI_Intercomm_merge
#endif


/*@
MPI_Intercomm_merge - Creates an intracommuncator from an intercommunicator

Input Parameters:
+ intercomm - Intercommunicator (handle)
- high - Used to order the groups within comm (logical)
  when creating the new communicator.  This is a boolean value; the group
  that sets high true has its processes ordered `after` the group that sets
  this value to false.  If all processes in the intercommunicator provide
  the same value, the choice of which group is ordered first is arbitrary.

Output Parameters:
. newintracomm - Created intracommunicator (handle)

Notes:
 While all processes may provide the same value for the 'high' parameter,
 this requires the MPI implementation to determine which group of
 processes should be ranked first.

.N ThreadSafe

.N Fortran

Algorithm:
.Eb
.i Allocate contexts
.i Local and remote group leaders swap high values
.i Determine the high value.
.i Merge the two groups and make the intra-communicator
.Ee

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_EXHAUSTED

.seealso: MPI_Intercomm_create, MPI_Comm_free
@*/
int MPI_Intercomm_merge(MPI_Comm intercomm, int high, MPI_Comm * newintracomm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Comm *new_intracomm_ptr;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_INTERCOMM_MERGE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_INTERCOMM_MERGE);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(intercomm, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPIR_Comm_get_ptr(intercomm, comm_ptr);

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            /* If comm_ptr is not valid, it will be reset to null */
            if (comm_ptr && comm_ptr->comm_kind != MPIR_COMM_KIND__INTERCOMM) {
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                                                 MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                                 MPI_ERR_COMM, "**commnotinter", 0);
            }
            if (mpi_errno)
                goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* Make sure that we have a local intercommunicator */
    if (!comm_ptr->local_comm) {
        /* Manufacture the local communicator */
        MPII_Setup_intercomm_localcomm(comm_ptr);
    }
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            int acthigh;
            MPIR_Errflag_t errflag = MPIR_ERR_NONE;
            /* Check for consistent valus of high in each local group.
             * The Intel test suite checks for this; it is also an easy
             * error to make */
            acthigh = high ? 1 : 0;     /* Clamp high into 1 or 0 */
            mpi_errno = MPIR_Allreduce(MPI_IN_PLACE, &acthigh, 1, MPI_INT,
                                       MPI_SUM, comm_ptr->local_comm, &errflag);
            MPIR_ERR_CHECK(mpi_errno);
            MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
            /* acthigh must either == 0 or the size of the local comm */
            if (acthigh != 0 && acthigh != comm_ptr->local_size) {
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                                                 MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                                 MPI_ERR_ARG, "**notsame", "**notsame %s %s",
                                                 "high", "MPI_Intercomm_merge");
                goto fn_fail;
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */

    mpi_errno = MPIR_Intercomm_merge_impl(comm_ptr, high, &new_intracomm_ptr);
    if (mpi_errno)
        goto fn_fail;

    MPIR_OBJ_PUBLISH_HANDLE(*newintracomm, new_intracomm_ptr->handle);

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_INTERCOMM_MERGE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_intercomm_merge", "**mpi_intercomm_merge %C %d %p",
                                 intercomm, high, newintracomm);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
