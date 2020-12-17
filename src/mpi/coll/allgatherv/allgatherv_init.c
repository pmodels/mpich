/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  Copyright (C) by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#include "mpiimpl.h"
#include "tsp_gentran.h"
#include "gentran_utils.h"

/* -- Begin Profiling Symbol Block for routine MPIX_Allgatherv_init */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_Allgatherv_init = PMPIX_Allgatherv_init
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_Allgatherv_init  MPIX_Allgatherv_init
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_Allgatherv_init as PMPIX_Allgatherv_init
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPIX_Allgatherv_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                         void *recvbuf, const int recvcounts[], const int displs[],
                         MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request * request)
    __attribute__ ((weak, alias("PMPIX_Allgatherv_init")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_Allgatherv_init
#define MPIX_Allgatherv_init PMPIX_Allgatherv_init

int MPIR_Allgatherv_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                         void *recvbuf, const int recvcounts[], const int displs[],
                         MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                         MPIR_Info * info_ptr, MPIR_Request ** request)
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
        /* intracommunicator */
        switch (MPIR_CVAR_IALLGETHERVV_INTRA_ALGORITHM) {
            case MPIR_CVAR_IALLGETHERVV_INTRA_ALGORITHM_gentran_recexch_doubling:
                /* This algo cannot handle unordered data */
                MPII_COLLECTIVE_FALLBACK_CHECK(comm_ptr->rank,
                                               MPII_Iallgatherv_is_displs_ordered
                                               (comm_size, recvcounts, displs), mpi_errno,
                                               "Iallgatherv gentran_recexch_doubling cannot be applied.\n");
                mpi_errno =
                    MPIR_Iallgatherv_intra_gentran_recexch_doubling(sendbuf, sendcount, sendtype,
                                                                    recvbuf, recvcounts, displs,
                                                                    recvtype, comm_ptr,
                                                                    MPIR_CVAR_IALLGETHERVV_RECEXCH_KVAL,
                                                                    request);
                break;

            case MPIR_CVAR_IALLGETHERVV_INTRA_ALGORITHM_gentran_recexch_halving:
                /* This algo cannot handle unordered data */
                MPII_COLLECTIVE_FALLBACK_CHECK(comm_ptr->rank,
                                               MPII_Iallgatherv_is_displs_ordered
                                               (comm_size, recvcounts, displs), mpi_errno,
                                               "Iallgatherv gentran_recexch_halving cannot be applied.\n");
                mpi_errno =
                    MPIR_Iallgatherv_intra_gentran_recexch_halving(sendbuf, sendcount, sendtype,
                                                                   recvbuf, recvcounts, displs,
                                                                   recvtype, comm_ptr,
                                                                   MPIR_CVAR_IALLGETHERVV_RECEXCH_KVAL,
                                                                   request);
                break;

            case MPIR_CVAR_IALLGETHERVV_INTRA_ALGORITHM_gentran_ring:
                mpi_errno =
                    MPIR_Iallgatherv_intra_gentran_ring(sendbuf, sendcount, sendtype,
                                                        recvbuf, recvcounts, displs,
                                                        recvtype, comm_ptr, request);
                break;

            case MPIR_CVAR_IALLGETHERVV_INTRA_ALGORITHM_gentran_brucks:
                mpi_errno =
                    MPIR_Iallgatherv_intra_gentran_brucks(sendbuf, sendcount, sendtype, recvbuf,
                                                          recvcounts, displs, recvtype, comm_ptr,
                                                          MPIR_CVAR_IALLGETHERVV_BRUCKS_KVAL,
                                                          request);
                break;

            case MPIR_CVAR_IALLGETHERVV_INTRA_ALGORITHM_sched_brucks:
                mpi_errno = MPIR_Iallgatherv_intra_sched_brucks(comm_ptr, request, sendbuf,
                                                                sendcount, sendtype, recvbuf,
                                                                recvcounts, displs, recvtype);
                break;

            case MPIR_CVAR_IALLGETHERVV_INTRA_ALGORITHM_sched_recursive_doubling:
                mpi_errno =
                    MPIR_Iallgatherv_intra_sched_recursive_doubling(comm_ptr, request, sendbuf,
                                                                    sendcount, sendtype, recvbuf,
                                                                    recvcounts, displs, recvtype);
                break;

            case MPIR_CVAR_IALLGETHERVV_INTRA_ALGORITHM_sched_ring:
                mpi_errno = MPIR_Iallgatherv_intra_sched_ring(comm_ptr, request, sendbuf, sendcount,
                                                              sendtype, recvbuf, recvcounts, displs,
                                                              recvtype);
                break;

            case MPIR_CVAR_IALLGETHERVV_INTRA_ALGORITHM_sched_auto:
                mpi_errno = MPIR_Iallgatherv_intra_sched_auto(comm_ptr, request, sendbuf, sendcount,
                                                              sendtype, recvbuf, recvcounts, displs,
                                                              recvtype);
                break;

            case MPIR_CVAR_IALLGETHERVV_INTRA_ALGORITHM_auto:
                mpi_errno = MPIR_Iallgatherv_allcomm_auto(sendbuf, sendcount, sendtype, recvbuf,
                                                          recvcounts, displs, recvtype, comm_ptr,
                                                          request);
                break;

            default:
                MPIR_Assert(0);
        }
    } else {
        switch (MPIR_CVAR_IALLGETHERVV_INTER_ALGORITHM) {
            case MPIR_CVAR_IALLGETHERVV_INTER_ALGORITHM_sched_remote_gather_local_bcast:
                mpi_errno =
                    MPIR_Iallgatherv_inter_sched_remote_gather_local_bcast(comm_ptr, request,
                                                                           sendbuf, sendcount,
                                                                           sendtype, recvbuf,
                                                                           recvcounts, displs,
                                                                           recvtype);
                break;

            case MPIR_CVAR_IALLGETHERVV_INTER_ALGORITHM_sched_auto:
                mpi_errno = MPIR_Iallgatherv_inter_sched_auto(comm_ptr, request, sendbuf, sendcount,
                                                              sendtype, recvbuf, recvcounts, displs,
                                                              recvtype);
                break;

            case MPIR_CVAR_IALLGETHERVV_INTER_ALGORITHM_auto:
                mpi_errno = MPIR_Iallgatherv_allcomm_auto(sendbuf, sendcount, sendtype, recvbuf,
                                                          recvcounts, displs, recvtype, comm_ptr,
                                                          request);
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
  fallback:
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        mpi_errno = MPIR_Iallgatherv_intra_sched_auto(comm_ptr, request, sendbuf, sendcount,
                                                      sendtype, recvbuf, recvcounts, displs,
                                                      recvtype);
    } else {
        mpi_errno = MPIR_Iallgatherv_inter_sched_auto(comm_ptr, request, sendbuf, sendcount,
                                                      sendtype, recvbuf, recvcounts, displs,
                                                      recvtype);
    }
    MPIR_ERR_CHECK(mpi_errno);
    req->u.persist.sched = sched;

    *request = req;
    goto fn_exit;

}

#endif

/*@
MPIX_Allgatherv_init - Creates a nonblocking, persistent collective communication request for the barrier operation.

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
int MPIX_Allgatherv_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                         const int recvcounts[], const int displs[], MPI_Datatype recvtype,
                         MPI_Comm comm, MPI_Info info, MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Info *info_ptr = NULL;
    MPIR_Request *request_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPIX_ALLGETHERV_INIT);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_PT2PT_ENTER(MPID_STATE_MPIX_ALLGETHERV_INIT);

    /* Validate handle parameters needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (sendbuf != MPI_IN_PLACE) {
                MPIR_ERRTEST_DATATYPE(sendtype, "sendtype", mpi_errno);
                MPIR_ERRTEST_COUNT(sendcount, mpi_errno);
            }
            MPIR_ERRTEST_DATATYPE(recvtype, "recvtype", mpi_errno);
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
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;

            if (sendbuf != MPI_IN_PLACE) {
                if (!HANDLE_IS_BUILTIN(sendtype)) {
                    MPIR_Datatype *sendtype_ptr = NULL;
                    MPIR_Datatype_get_ptr(sendtype, sendtype_ptr);
                    MPIR_Datatype_valid_ptr(sendtype_ptr, mpi_errno);
                    if (mpi_errno != MPI_SUCCESS)
                        goto fn_fail;
                    MPIR_Datatype_committed_ptr(sendtype_ptr, mpi_errno);
                    if (mpi_errno != MPI_SUCCESS)
                        goto fn_fail;
                }

                /* catch common aliasing cases */
                if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM &&
                    sendtype == recvtype && recvcounts[comm_ptr->rank] != 0 && sendcount != 0) {
                    int recvtype_size;
                    MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
                    MPIR_ERRTEST_ALIAS_COLL(sendbuf,
                                            (char *) recvbuf +
                                            displs[comm_ptr->rank] * recvtype_size, mpi_errno);
                }
            }

            MPIR_ERRTEST_ARGNULL(recvcounts, "recvcounts", mpi_errno);
            MPIR_ERRTEST_ARGNULL(displs, "displs", mpi_errno);
            if (!HANDLE_IS_BUILTIN(recvtype)) {
                MPIR_Datatype *recvtype_ptr = NULL;
                MPIR_Datatype_get_ptr(recvtype, recvtype_ptr);
                MPIR_Datatype_valid_ptr(recvtype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_fail;
                MPIR_Datatype_committed_ptr(recvtype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_fail;
            }

            MPIR_ERRTEST_ARGNULL(request, "request", mpi_errno);
            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    if (MPIR_CVAR_IALLGETHERV_DEVICE_COLLECTIVE && MPIR_CVAR_DEVICE_COLLECTIVES)
        mpi_errno = MPID_Allgatherv_init(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs,
                                         recvtype, comm_ptr, info_ptr, &request_ptr);
    else
        mpi_errno = MPIR_Allgatherv_init(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs,
                                         recvtype, comm_ptr, info_ptr, &request_ptr);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    /* return the handle of the request to the user */
    MPIR_OBJ_PUBLISH_HANDLE(*request, request_ptr->handle);

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_PT2PT_EXIT(MPID_STATE_MPIX_ALLGETHERV_INIT);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpix_allgatherv_init",
                                 "**mpix_allgatherv_init %p %d %D %p %p %p %D %C %I %p",
                                 sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs,
                                 recvtype, comm, info, request);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
