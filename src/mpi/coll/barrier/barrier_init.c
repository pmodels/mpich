/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  Copyright (C) by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#include "mpiimpl.h"
#include "tsp_gentran.h"
#include "gentran_utils.h"

/* -- Begin Profiling Symbol Block for routine MPIX_Barrier_init */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_Barrier_init = PMPIX_Barrier_init
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_Barrier_init  MPIX_Barrier_init
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_Barrier_init as PMPIX_Barrier_init
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPIX_Barrier_init(MPI_Comm comm, MPI_Info info, MPI_Request * request)
    __attribute__ ((weak, alias("PMPIX_Barrier_init")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_Barrier_init
#define MPIX_Barrier_init PMPIX_Barrier_init

int MPIR_Barrier_init(MPIR_Comm * comm_ptr, MPIR_Info * info_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_TSP_sched_t *sched;

    /* create a new request */
    MPIR_Request *req = MPIR_Request_create(MPIR_REQUEST_KIND__PREQUEST_COLL);
    MPIR_ERR_CHKANDJUMP(!req, mpi_errno, MPI_ERR_OTHER, "**nomem");

    req->u.persist.real_request = NULL;

    /* generate the schedule */
    sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
    MPIR_ERR_CHKANDJUMP(!sched, mpi_errno, MPI_ERR_OTHER, "**nomem");
    MPIR_TSP_sched_create(sched, true);

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        switch (MPIR_CVAR_IBARRIER_INTRA_ALGORITHM) {
            case MPIR_CVAR_IBARRIER_INTRA_ALGORITHM_gentran_recexch:
                mpi_errno =
                    MPIR_Ibarrier_intra_gentran_recexch(comm_ptr, MPIR_CVAR_IBARRIER_RECEXCH_KVAL,
                                                        request);

                break;

            case MPIR_CVAR_IBARRIER_INTRA_ALGORITHM_sched_recursive_doubling:
                mpi_errno = MPIR_Ibarrier_intra_sched_recursive_doubling(comm_ptr, request);
                break;

            case MPIR_CVAR_IBARRIER_INTRA_ALGORITHM_sched_auto:
                mpi_errno = MPIR_Ibarrier_intra_sched_auto(comm_ptr, request);
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
                mpi_errno = MPIR_Ibarrier_inter_sched_bcast(comm_ptr, request);
                break;

            case MPIR_CVAR_IBARRIER_INTER_ALGORITHM_sched_auto:
                mpi_errno = MPIR_Ibarrier_inter_sched_auto(comm_ptr, request);
                break;

            case MPIR_CVAR_IBARRIER_INTER_ALGORITHM_auto:
                mpi_errno = MPIR_Ibarrier_allcomm_auto(comm_ptr, request);
                break;

            default:
                MPIR_Assert(0);
        }
    }

    MPIR_ERR_CHECK(mpi_errno);
    req->u.persist.sched = sched;

    *request = req;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif

/*@
MPIX_Barrier_init - Creates a nonblocking, persistent collective communication request for the barrier operation.

Input Parameters:
. comm - communicator (handle)
. info - info argument (handle)
- request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM

.seealso: MPI_Start, MPI_Startall, MPI_Request_free
@*/
int MPIX_Barrier_init(MPI_Comm comm, MPI_Info info, MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Info *info_ptr = NULL;
    MPIR_Request *request_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPIX_BARRIER_INIT);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_PT2PT_ENTER(MPID_STATE_MPIX_BARRIER_INIT);

    /* Validate handle parameters needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPIR_Comm_get_ptr(comm, comm_ptr);
    MPIR_Info_get_ptr(info, info_ptr);

    /* Validate parameters if error checking is enabled */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            MPIR_Info_valid_ptr(info_ptr, mpi_errno);

            if (mpi_errno)
                goto fn_fail;

            MPIR_ERRTEST_ARGNULL(request, "request", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    if (MPIR_CVAR_IBARRIER_DEVICE_COLLECTIVE && MPIR_CVAR_DEVICE_COLLECTIVES)
        mpi_errno = MPID_Barrier_init(comm_ptr, info_ptr, &request_ptr);
    else
        mpi_errno = MPIR_Barrier_init(comm_ptr, info_ptr, &request_ptr);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    /* return the handle of the request to the user */
    MPIR_OBJ_PUBLISH_HANDLE(*request, request_ptr->handle);

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_PT2PT_EXIT(MPID_STATE_MPIX_BARRIER_INIT);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpix_barrier_init", "**mpix_barrier_init %C %I %p",
                                 comm, info, request);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
