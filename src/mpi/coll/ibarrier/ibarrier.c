/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_IBARRIER_RECEXCH_KVAL
      category    : COLLECTIVE
      type        : int
      default     : 2
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        k value for recursive exchange based ibarrier

    - name        : MPIR_CVAR_IBARRIER_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select ibarrier algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        sched_auto - Internal algorithm selection for sched-based algorithms
        sched_recursive_doubling - Force recursive doubling algorithm
        gentran_recexch          - Force generic transport based recursive exchange algorithm

    - name        : MPIR_CVAR_IBARRIER_INTER_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select ibarrier algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        sched_auto - Internal algorithm selection for sched-based algorithms
        sched_bcast - Force bcast algorithm

    - name        : MPIR_CVAR_IBARRIER_DEVICE_COLLECTIVE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        This CVAR is only used when MPIR_CVAR_DEVICE_COLLECTIVES
        is set to "percoll".  If set to true, MPI_Ibarrier will
        allow the device to override the MPIR-level collective
        algorithms.  The device might still call the MPIR-level
        algorithms manually.  If set to false, the device-override
        will be disabled.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* -- Begin Profiling Symbol Block for routine MPI_Ibarrier */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Ibarrier = PMPI_Ibarrier
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Ibarrier  MPI_Ibarrier
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Ibarrier as PMPI_Ibarrier
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Ibarrier(MPI_Comm comm, MPI_Request * request)
    __attribute__ ((weak, alias("PMPI_Ibarrier")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Ibarrier
#define MPI_Ibarrier PMPI_Ibarrier

/* any non-MPI functions go here, especially non-static ones */


int MPIR_Ibarrier_allcomm_auto(MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__IBARRIER,
        .comm_ptr = comm_ptr,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibarrier_intra_sched_auto:
            MPII_SCHED_WRAPPER_EMPTY(MPIR_Ibarrier_intra_sched_auto, comm_ptr, request);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibarrier_intra_sched_recursive_doubling:
            MPII_SCHED_WRAPPER_EMPTY(MPIR_Ibarrier_intra_sched_recursive_doubling, comm_ptr,
                                     request);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibarrier_intra_gentran_recexch:
            mpi_errno =
                MPIR_Ibarrier_intra_gentran_recexch(comm_ptr,
                                                    cnt->u.ibarrier.intra_gentran_recexch.k,
                                                    request);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibarrier_inter_sched_auto:
            MPII_SCHED_WRAPPER_EMPTY(MPIR_Ibarrier_inter_sched_auto, comm_ptr, request);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibarrier_inter_sched_bcast:
            MPII_SCHED_WRAPPER_EMPTY(MPIR_Ibarrier_inter_sched_bcast, comm_ptr, request);
            break;

        default:
            MPIR_Assert(0);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Ibarrier_intra_sched_auto(MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Ibarrier_intra_sched_recursive_doubling(comm_ptr, s);

    return mpi_errno;
}

/* It will choose between several different algorithms based on the given
 * parameters. */
int MPIR_Ibarrier_inter_sched_auto(MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno;

    mpi_errno = MPIR_Ibarrier_inter_sched_bcast(comm_ptr, s);

    return mpi_errno;
}

int MPIR_Ibarrier_sched_auto(MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        mpi_errno = MPIR_Ibarrier_intra_sched_auto(comm_ptr, s);
    } else {
        mpi_errno = MPIR_Ibarrier_inter_sched_auto(comm_ptr, s);
    }

    return mpi_errno;
}

int MPIR_Ibarrier_impl(MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    *request = NULL;

    /* If the user picks one of the transport-enabled algorithms, branch there
     * before going down to the MPIR_Sched-based algorithms. */
    /* TODO - Eventually the intention is to replace all of the
     * MPIR_Sched-based algorithms with transport-enabled algorithms, but that
     * will require sufficient performance testing and replacement algorithms. */
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        switch (MPIR_CVAR_IBARRIER_INTRA_ALGORITHM) {
            case MPIR_CVAR_IBARRIER_INTRA_ALGORITHM_gentran_recexch:
                mpi_errno =
                    MPIR_Ibarrier_intra_gentran_recexch(comm_ptr, MPIR_CVAR_IBARRIER_RECEXCH_KVAL,
                                                        request);

                break;

            case MPIR_CVAR_IBARRIER_INTRA_ALGORITHM_sched_recursive_doubling:
                MPII_SCHED_WRAPPER_EMPTY(MPIR_Ibarrier_intra_sched_recursive_doubling, comm_ptr,
                                         request);
                break;

            case MPIR_CVAR_IBARRIER_INTRA_ALGORITHM_sched_auto:
                MPII_SCHED_WRAPPER_EMPTY(MPIR_Ibarrier_intra_sched_auto, comm_ptr, request);
                break;

            case MPIR_CVAR_IBARRIER_INTRA_ALGORITHM_auto:
                mpi_errno = MPIR_Ibarrier_allcomm_auto(comm_ptr, request);
                break;

            default:
                MPIR_Assert(0);
        }
    } else {
        switch (MPIR_CVAR_IBARRIER_INTER_ALGORITHM) {
            case MPIR_CVAR_IBARRIER_INTER_ALGORITHM_sched_bcast:
                MPII_SCHED_WRAPPER_EMPTY(MPIR_Ibarrier_inter_sched_bcast, comm_ptr, request);
                break;

            case MPIR_CVAR_IBARRIER_INTER_ALGORITHM_sched_auto:
                MPII_SCHED_WRAPPER_EMPTY(MPIR_Ibarrier_inter_sched_auto, comm_ptr, request);
                break;

            case MPIR_CVAR_IBARRIER_INTER_ALGORITHM_auto:
                mpi_errno = MPIR_Ibarrier_allcomm_auto(comm_ptr, request);
                break;

            default:
                MPIR_Assert(0);
        }
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Ibarrier(MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_IBARRIER_DEVICE_COLLECTIVE)) {
        mpi_errno = MPID_Ibarrier(comm_ptr, request);
    } else {
        mpi_errno = MPIR_Ibarrier_impl(comm_ptr, request);
    }

    return mpi_errno;
}

#endif /* MPICH_MPI_FROM_PMPI */

/*@
MPI_Ibarrier - Notifies the process that it has reached the barrier and returns
               immediately

Input Parameters:
. comm - communicator (handle)

Output Parameters:
. request - communication request (handle)

Notes:
MPI_Ibarrier is a nonblocking version of MPI_barrier. By calling MPI_Ibarrier,
a process notifies that it has reached the barrier. The call returns
immediately, independent of whether other processes have called MPI_Ibarrier.
The usual barrier semantics are enforced at the corresponding completion
operation (test or wait), which in the intra-communicator case will complete
only after all other processes in the communicator have called MPI_Ibarrier. In
the intercommunicator case, it will complete when all processes in the remote
group have called MPI_Ibarrier.

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_Ibarrier(MPI_Comm comm, MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Request *request_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_IBARRIER);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_IBARRIER);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);

            /* TODO more checks may be appropriate */
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPIR_Comm_get_ptr(comm, comm_ptr);

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;
            MPIR_ERRTEST_ARGNULL(request, "request", mpi_errno);
            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Ibarrier(comm_ptr, &request_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    /* create a complete request, if needed */
    if (!request_ptr)
        request_ptr = MPIR_Request_create_complete(MPIR_REQUEST_KIND__COLL);
    /* return the handle of the request to the user */
    *request = request_ptr->handle;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_IBARRIER);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_ibarrier", "**mpi_ibarrier %C %p", comm, request);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
