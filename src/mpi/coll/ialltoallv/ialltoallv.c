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
