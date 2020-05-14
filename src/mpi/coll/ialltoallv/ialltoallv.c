/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_IALLTOALLV_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select ialltoallv algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        sched_auto - Internal algorithm selection for sched-based algorithms
        sched_blocked           - Force blocked algorithm
        sched_inplace           - Force inplace algorithm
        sched_pairwise_exchange - Force pairwise exchange algorithm
        gentran_scattered       - Force generic transport based scattered algorithm
        gentran_blocked         - Force generic transport blocked algorithm
        gentran_inplace         - Force generic transport inplace algorithm

    - name        : MPIR_CVAR_IALLTOALLV_INTER_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select ialltoallv algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        sched_auto - Internal algorithm selection for sched-based algorithms
        sched_pairwise_exchange - Force pairwise exchange algorithm

    - name        : MPIR_CVAR_IALLTOALLV_DEVICE_COLLECTIVE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        This CVAR is only used when MPIR_CVAR_DEVICE_COLLECTIVES
        is set to "percoll".  If set to true, MPI_Ialltoallv will
        allow the device to override the MPIR-level collective
        algorithms.  The device might still call the MPIR-level
        algorithms manually.  If set to false, the device-override
        will be disabled.

    - name        : MPIR_CVAR_IALLTOALLV_SCATTERED_OUTSTANDING_TASKS
      category    : COLLECTIVE
      type        : int
      default     : 64
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Maximum number of outstanding sends and recvs posted at a time

    - name        : MPIR_CVAR_IALLTOALLV_SCATTERED_BATCH_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 4
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Number of send/receive tasks that scattered algorithm waits for
        completion before posting another batch of send/receives of that size

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* -- Begin Profiling Symbol Block for routine MPI_Ialltoallv */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Ialltoallv = PMPI_Ialltoallv
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Ialltoallv  MPI_Ialltoallv
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Ialltoallv as PMPI_Ialltoallv
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Ialltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[],
                   MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                   const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request * request)
    __attribute__ ((weak, alias("PMPI_Ialltoallv")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Ialltoallv
#define MPI_Ialltoallv PMPI_Ialltoallv

/* any non-MPI functions go here, especially non-static ones */

int MPIR_Ialltoallv_allcomm_auto(const void *sendbuf, const int *sendcounts, const int *sdispls,
                                 MPI_Datatype sendtype, void *recvbuf, const int *recvcounts,
                                 const int *rdispls, MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                 MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__IALLTOALLV,
        .comm_ptr = comm_ptr,

        .u.ialltoallv.sendbuf = sendbuf,
        .u.ialltoallv.sendcounts = sendcounts,
        .u.ialltoallv.sdispls = sdispls,
        .u.ialltoallv.sendtype = sendtype,
        .u.ialltoallv.recvbuf = recvbuf,
        .u.ialltoallv.recvcounts = recvcounts,
        .u.ialltoallv.rdispls = rdispls,
        .u.ialltoallv.recvtype = recvtype,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallv_intra_sched_auto:
            MPII_SCHED_WRAPPER(MPIR_Ialltoallv_intra_sched_auto, comm_ptr, request, sendbuf,
                               sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls,
                               recvtype);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallv_intra_sched_blocked:
            MPII_SCHED_WRAPPER(MPIR_Ialltoallv_intra_sched_blocked, comm_ptr, request, sendbuf,
                               sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls,
                               recvtype);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallv_intra_sched_inplace:
            MPII_SCHED_WRAPPER(MPIR_Ialltoallv_intra_sched_inplace, comm_ptr, request, sendbuf,
                               sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls,
                               recvtype);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallv_intra_gentran_scattered:
            mpi_errno =
                MPIR_Ialltoallv_intra_gentran_scattered(sendbuf, sendcounts, sdispls, sendtype,
                                                        recvbuf, recvcounts, rdispls, recvtype,
                                                        comm_ptr,
                                                        cnt->u.ialltoallv.
                                                        intra_gentran_scattered.batch_size,
                                                        cnt->u.ialltoallv.
                                                        intra_gentran_scattered.bblock, request);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallv_intra_gentran_blocked:
            mpi_errno =
                MPIR_Ialltoallv_intra_gentran_blocked(sendbuf, sendcounts, sdispls, sendtype,
                                                      recvbuf, recvcounts, rdispls, recvtype,
                                                      comm_ptr,
                                                      cnt->u.ialltoallv.
                                                      intra_gentran_blocked.bblock, request);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallv_intra_gentran_inplace:
            mpi_errno =
                MPIR_Ialltoallv_intra_gentran_inplace(sendbuf, sendcounts, sdispls, sendtype,
                                                      recvbuf, recvcounts, rdispls, recvtype,
                                                      comm_ptr, request);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallv_inter_sched_auto:
            MPII_SCHED_WRAPPER(MPIR_Ialltoallv_inter_sched_auto, comm_ptr, request, sendbuf,
                               sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls,
                               recvtype);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallv_inter_sched_pairwise_exchange:
            MPII_SCHED_WRAPPER(MPIR_Ialltoallv_inter_sched_pairwise_exchange, comm_ptr, request,
                               sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls,
                               recvtype);
            break;

        default:
            MPIR_Assert(0);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Ialltoallv_intra_sched_auto(const void *sendbuf, const int sendcounts[],
                                     const int sdispls[], MPI_Datatype sendtype, void *recvbuf,
                                     const int recvcounts[], const int rdispls[],
                                     MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);

    if (sendbuf == MPI_IN_PLACE) {
        mpi_errno = MPIR_Ialltoallv_intra_sched_inplace(sendbuf, sendcounts, sdispls,
                                                        sendtype, recvbuf, recvcounts,
                                                        rdispls, recvtype, comm_ptr, s);
    } else {
        mpi_errno = MPIR_Ialltoallv_intra_sched_blocked(sendbuf, sendcounts, sdispls,
                                                        sendtype, recvbuf, recvcounts,
                                                        rdispls, recvtype, comm_ptr, s);
    }

    return mpi_errno;
}

int MPIR_Ialltoallv_inter_sched_auto(const void *sendbuf, const int sendcounts[],
                                     const int sdispls[], MPI_Datatype sendtype, void *recvbuf,
                                     const int recvcounts[], const int rdispls[],
                                     MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Ialltoallv_inter_sched_pairwise_exchange(sendbuf, sendcounts, sdispls,
                                                              sendtype, recvbuf, recvcounts,
                                                              rdispls, recvtype, comm_ptr, s);

    return mpi_errno;
}

int MPIR_Ialltoallv_sched_auto(const void *sendbuf, const int sendcounts[], const int sdispls[],
                               MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                               const int rdispls[], MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                               MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        mpi_errno = MPIR_Ialltoallv_intra_sched_auto(sendbuf, sendcounts, sdispls,
                                                     sendtype, recvbuf, recvcounts, rdispls,
                                                     recvtype, comm_ptr, s);
    } else {
        mpi_errno = MPIR_Ialltoallv_inter_sched_auto(sendbuf, sendcounts, sdispls,
                                                     sendtype, recvbuf, recvcounts, rdispls,
                                                     recvtype, comm_ptr, s);
    }

    return mpi_errno;
}

int MPIR_Ialltoallv_impl(const void *sendbuf, const int sendcounts[], const int sdispls[],
                         MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                         const int rdispls[], MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                         MPIR_Request ** request)
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
        switch (MPIR_CVAR_IALLTOALLV_INTRA_ALGORITHM) {
            case MPIR_CVAR_IALLTOALLV_INTRA_ALGORITHM_gentran_scattered:
                MPII_COLLECTIVE_FALLBACK_CHECK(comm_ptr->rank, sendbuf != MPI_IN_PLACE, mpi_errno,
                                               "Ialltoallv gentran_scattered cannot be applied.\n");
                mpi_errno =
                    MPIR_Ialltoallv_intra_gentran_scattered(sendbuf, sendcounts, sdispls,
                                                            sendtype, recvbuf, recvcounts,
                                                            rdispls, recvtype, comm_ptr,
                                                            MPIR_CVAR_IALLTOALLV_SCATTERED_BATCH_SIZE,
                                                            MPIR_CVAR_IALLTOALLV_SCATTERED_OUTSTANDING_TASKS,
                                                            request);
                break;

            case MPIR_CVAR_IALLTOALLV_INTRA_ALGORITHM_gentran_blocked:
                MPII_COLLECTIVE_FALLBACK_CHECK(comm_ptr->rank, sendbuf != MPI_IN_PLACE, mpi_errno,
                                               "Ialltoallv gentran_blocked cannot be applied.\n");
                mpi_errno =
                    MPIR_Ialltoallv_intra_gentran_blocked(sendbuf, sendcounts, sdispls,
                                                          sendtype, recvbuf, recvcounts,
                                                          rdispls, recvtype, comm_ptr,
                                                          MPIR_CVAR_ALLTOALL_THROTTLE, request);
                break;

            case MPIR_CVAR_IALLTOALLV_INTRA_ALGORITHM_gentran_inplace:
                MPII_COLLECTIVE_FALLBACK_CHECK(comm_ptr->rank, sendbuf == MPI_IN_PLACE, mpi_errno,
                                               "Ialltoallv gentran_inplace cannot be applied.\n");
                mpi_errno =
                    MPIR_Ialltoallv_intra_gentran_inplace(sendbuf, sendcounts, sdispls,
                                                          sendtype, recvbuf, recvcounts,
                                                          rdispls, recvtype, comm_ptr, request);
                break;

            case MPIR_CVAR_IALLTOALLV_INTRA_ALGORITHM_sched_blocked:
                MPII_SCHED_WRAPPER(MPIR_Ialltoallv_intra_sched_blocked, comm_ptr, request, sendbuf,
                                   sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls,
                                   recvtype);
                break;

            case MPIR_CVAR_IALLTOALLV_INTRA_ALGORITHM_sched_inplace:
                MPII_COLLECTIVE_FALLBACK_CHECK(comm_ptr->rank, sendbuf != MPI_IN_PLACE, mpi_errno,
                                               "Ialltoallv sched_inplace cannot be applied.\n");
                MPII_SCHED_WRAPPER(MPIR_Ialltoallv_intra_sched_inplace, comm_ptr, request, sendbuf,
                                   sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls,
                                   recvtype);
                break;

            case MPIR_CVAR_IALLTOALLV_INTRA_ALGORITHM_sched_auto:
                MPII_SCHED_WRAPPER(MPIR_Ialltoallv_intra_sched_auto, comm_ptr, request, sendbuf,
                                   sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls,
                                   recvtype);
                break;

            case MPIR_CVAR_IALLTOALLV_INTRA_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Ialltoallv_allcomm_auto(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                                                 recvcounts, rdispls, recvtype, comm_ptr, request);
                break;

            default:
                MPIR_Assert(0);
        }
    } else {
        switch (MPIR_CVAR_IALLTOALLV_INTER_ALGORITHM) {
            case MPIR_CVAR_IALLTOALLV_INTER_ALGORITHM_sched_pairwise_exchange:
                MPII_SCHED_WRAPPER(MPIR_Ialltoallv_inter_sched_pairwise_exchange, comm_ptr, request,
                                   sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts,
                                   rdispls, recvtype);
                break;

            case MPIR_CVAR_IALLTOALLV_INTER_ALGORITHM_sched_auto:
                MPII_SCHED_WRAPPER(MPIR_Ialltoallv_inter_sched_auto, comm_ptr, request, sendbuf,
                                   sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls,
                                   recvtype);
                break;

            case MPIR_CVAR_IALLTOALLV_INTER_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Ialltoallv_allcomm_auto(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                                                 recvcounts, rdispls, recvtype, comm_ptr, request);
                break;

            default:
                MPIR_Assert(0);
        }
    }

    MPIR_ERR_CHECK(mpi_errno);
    goto fn_exit;

  fallback:
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        MPII_SCHED_WRAPPER(MPIR_Ialltoallv_intra_sched_auto, comm_ptr, request, sendbuf,
                           sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype);
    } else {
        MPII_SCHED_WRAPPER(MPIR_Ialltoallv_inter_sched_auto, comm_ptr, request, sendbuf,
                           sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Ialltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[],
                    MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                    const int rdispls[], MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                    MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_IALLTOALLV_DEVICE_COLLECTIVE)) {
        mpi_errno =
            MPID_Ialltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls,
                            recvtype, comm_ptr, request);
    } else {
        mpi_errno = MPIR_Ialltoallv_impl(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                                         recvcounts, rdispls, recvtype, comm_ptr, request);
    }

    return mpi_errno;
}

#endif /* MPICH_MPI_FROM_PMPI */

/*@
MPI_Ialltoallv - Sends data from all to all processes in a nonblocking way;
   each process may send a different amount of data and provide displacements
   for the input and output data.

Input Parameters:
+ sendbuf - starting address of the send buffer (choice)
. sendcounts - non-negative integer array (of length group size) specifying the number of elements to send to each processor
. sdispls - integer array (of length group size). Entry j specifies the displacement relative to sendbuf from which to take the outgoing data destined for process j
. sendtype - data type of send buffer elements (handle)
. recvcounts - non-negative integer array (of length group size) specifying the number of elements that can be received from each processor
. rdispls - integer array (of length group size). Entry i specifies the displacement relative to recvbuf at which to place the incoming data from process i
. recvtype - data type of receive buffer elements (handle)
- comm - communicator (handle)

Output Parameters:
+ recvbuf - starting address of the receive buffer (choice)
- request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_Ialltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[],
                   MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                   const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Request *request_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_IALLTOALLV);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_IALLTOALLV);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (sendbuf != MPI_IN_PLACE)
                MPIR_ERRTEST_DATATYPE(sendtype, "sendtype", mpi_errno);
            MPIR_ERRTEST_DATATYPE(recvtype, "recvtype", mpi_errno);
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

            if (sendbuf != MPI_IN_PLACE) {
                MPIR_ERRTEST_ARGNULL(sendcounts, "sendcounts", mpi_errno);
                MPIR_ERRTEST_ARGNULL(sdispls, "sdispls", mpi_errno);
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
            }

            MPIR_ERRTEST_ARGNULL(recvcounts, "recvcounts", mpi_errno);
            MPIR_ERRTEST_ARGNULL(rdispls, "rdispls", mpi_errno);
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

            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM &&
                sendbuf != MPI_IN_PLACE && sendcounts == recvcounts && sendtype == recvtype)
                MPIR_ERRTEST_ALIAS_COLL(sendbuf, recvbuf, mpi_errno);
            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Ialltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                                recvcounts, rdispls, recvtype, comm_ptr, &request_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    /* create a complete request, if needed */
    if (!request_ptr)
        request_ptr = MPIR_Request_create_complete(MPIR_REQUEST_KIND__COLL);
    /* return the handle of the request to the user */
    *request = request_ptr->handle;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_IALLTOALLV);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_ialltoallv",
                                 "**mpi_ialltoallv %p %p %p %D %p %p %p %D %C %p", sendbuf,
                                 sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls,
                                 recvtype, comm, request);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
