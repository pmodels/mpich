/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
/* for MPIR_TSP_sched_t */
#include "tsp_gentran.h"
#include "gentran_utils.h"
#include "../ialltoallv/ialltoallv_tsp_blocked_algos_prototypes.h"
#include "../ialltoallv/ialltoallv_tsp_inplace_algos_prototypes.h"
#include "../ialltoallv/ialltoallv_tsp_scattered_algos_prototypes.h"

/*
*/

/* any non-MPI functions go here, especially non-static ones */

int MPIR_Ialltoallv_allcomm_sched_auto(const void *sendbuf, const MPI_Aint * sendcounts,
                                       const MPI_Aint * sdispls, MPI_Datatype sendtype,
                                       void *recvbuf, const MPI_Aint * recvcounts,
                                       const MPI_Aint * rdispls, MPI_Datatype recvtype,
                                       MPIR_Comm * comm_ptr, bool is_persistent, void **sched_p,
                                       enum MPIR_sched_type *sched_type_p)
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
        /* *INDENT-OFF* */
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallv_intra_sched_blocked:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Ialltoallv_intra_sched_blocked(sendbuf, sendcounts, sdispls, sendtype,
                                                            recvbuf, recvcounts, rdispls, recvtype,
                                                            comm_ptr, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallv_intra_sched_inplace:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Ialltoallv_intra_sched_inplace(sendbuf, sendcounts, sdispls, sendtype,
                                                            recvbuf, recvcounts, rdispls, recvtype,
                                                            comm_ptr, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallv_intra_gentran_scattered:
            MPII_GENTRAN_CREATE_SCHED_P();
            mpi_errno =
                MPIR_TSP_Ialltoallv_sched_intra_scattered(sendbuf, sendcounts, sdispls, sendtype,
                                                          recvbuf, recvcounts, rdispls, recvtype,
                                                          comm_ptr,
                                                          cnt->u.ialltoallv.
                                                          intra_gentran_scattered.batch_size,
                                                          cnt->u.ialltoallv.
                                                          intra_gentran_scattered.bblock, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallv_intra_gentran_blocked:
            MPII_GENTRAN_CREATE_SCHED_P();
            mpi_errno =
                MPIR_TSP_Ialltoallv_sched_intra_blocked(sendbuf, sendcounts, sdispls, sendtype,
                                                        recvbuf, recvcounts, rdispls, recvtype,
                                                        comm_ptr,
                                                        cnt->u.ialltoallv.
                                                        intra_gentran_blocked.bblock, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallv_intra_gentran_inplace:
            MPII_GENTRAN_CREATE_SCHED_P();
            mpi_errno =
                MPIR_TSP_Ialltoallv_sched_intra_inplace(sendbuf, sendcounts, sdispls, sendtype,
                                                        recvbuf, recvcounts, rdispls, recvtype,
                                                        comm_ptr, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoallv_inter_sched_pairwise_exchange:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Ialltoallv_inter_sched_pairwise_exchange(sendbuf, sendcounts, sdispls,
                                                                      sendtype, recvbuf, recvcounts,
                                                                      rdispls, recvtype, comm_ptr,
                                                                      *sched_p);
            break;

        default:
            MPIR_Assert(0);
        /* *INDENT-ON* */
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Ialltoallv_sched_impl(const void *sendbuf, const MPI_Aint sendcounts[],
                               const MPI_Aint sdispls[], MPI_Datatype sendtype, void *recvbuf,
                               const MPI_Aint recvcounts[], const MPI_Aint rdispls[],
                               MPI_Datatype recvtype, MPIR_Comm * comm_ptr, bool is_persistent,
                               void **sched_p, enum MPIR_sched_type *sched_type_p)
{
    int mpi_errno = MPI_SUCCESS;

    /* If the user picks one of the transport-enabled algorithms, branch there
     * before going down to the MPIR_Sched-based algorithms. */
    /* TODO - Eventually the intention is to replace all of the
     * MPIR_Sched-based algorithms with transport-enabled algorithms, but that
     * will require sufficient performance testing and replacement algorithms. */
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        switch (MPIR_CVAR_IALLTOALLV_INTRA_ALGORITHM) {
            /* *INDENT-OFF* */
            case MPIR_CVAR_IALLTOALLV_INTRA_ALGORITHM_gentran_scattered:
                MPII_COLLECTIVE_FALLBACK_CHECK(comm_ptr->rank, sendbuf != MPI_IN_PLACE, mpi_errno,
                                               "Ialltoallv gentran_scattered cannot be applied.\n");
                MPII_GENTRAN_CREATE_SCHED_P();
                mpi_errno =
                    MPIR_TSP_Ialltoallv_sched_intra_scattered(sendbuf, sendcounts, sdispls,
                                                              sendtype, recvbuf, recvcounts,
                                                              rdispls, recvtype, comm_ptr,
                                                              MPIR_CVAR_IALLTOALLV_SCATTERED_BATCH_SIZE,
                                                              MPIR_CVAR_IALLTOALLV_SCATTERED_OUTSTANDING_TASKS,
                                                              *sched_p);
                break;

            case MPIR_CVAR_IALLTOALLV_INTRA_ALGORITHM_gentran_blocked:
                MPII_COLLECTIVE_FALLBACK_CHECK(comm_ptr->rank, sendbuf != MPI_IN_PLACE, mpi_errno,
                                               "Ialltoallv gentran_blocked cannot be applied.\n");
                MPII_GENTRAN_CREATE_SCHED_P();
                mpi_errno =
                    MPIR_TSP_Ialltoallv_sched_intra_blocked(sendbuf, sendcounts, sdispls, sendtype,
                                                            recvbuf, recvcounts, rdispls, recvtype,
                                                            comm_ptr, MPIR_CVAR_ALLTOALL_THROTTLE,
                                                            *sched_p);
                break;

            case MPIR_CVAR_IALLTOALLV_INTRA_ALGORITHM_gentran_inplace:
                MPII_COLLECTIVE_FALLBACK_CHECK(comm_ptr->rank, sendbuf == MPI_IN_PLACE, mpi_errno,
                                               "Ialltoallv gentran_inplace cannot be applied.\n");
                MPII_GENTRAN_CREATE_SCHED_P();
                mpi_errno =
                    MPIR_TSP_Ialltoallv_sched_intra_inplace(sendbuf, sendcounts, sdispls, sendtype,
                                                            recvbuf, recvcounts, rdispls, recvtype,
                                                            comm_ptr, *sched_p);
                break;

            case MPIR_CVAR_IALLTOALLV_INTRA_ALGORITHM_sched_blocked:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ialltoallv_intra_sched_blocked(sendbuf, sendcounts, sdispls,
                                                                sendtype, recvbuf, recvcounts,
                                                                rdispls, recvtype, comm_ptr,
                                                                *sched_p);
                break;

            case MPIR_CVAR_IALLTOALLV_INTRA_ALGORITHM_sched_inplace:
                MPII_COLLECTIVE_FALLBACK_CHECK(comm_ptr->rank, sendbuf != MPI_IN_PLACE, mpi_errno,
                                               "Ialltoallv sched_inplace cannot be applied.\n");
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ialltoallv_intra_sched_inplace(sendbuf, sendcounts, sdispls,
                                                                sendtype, recvbuf, recvcounts,
                                                                rdispls, recvtype, comm_ptr,
                                                                *sched_p);
                break;

            case MPIR_CVAR_IALLTOALLV_INTRA_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Ialltoallv_allcomm_sched_auto(sendbuf, sendcounts, sdispls, sendtype,
                                                       recvbuf, recvcounts, rdispls, recvtype,
                                                       comm_ptr, is_persistent, sched_p,
                                                       sched_type_p);
                break;

            default:
                MPIR_Assert(0);
            /* *INDENT-ON* */
        }
    } else {
        switch (MPIR_CVAR_IALLTOALLV_INTER_ALGORITHM) {
            /* *INDENT-OFF* */
            case MPIR_CVAR_IALLTOALLV_INTER_ALGORITHM_sched_pairwise_exchange:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ialltoallv_inter_sched_pairwise_exchange(sendbuf, sendcounts,
                                                                          sdispls, sendtype,
                                                                          recvbuf, recvcounts,
                                                                          rdispls, recvtype,
                                                                          comm_ptr, *sched_p);
                break;

            case MPIR_CVAR_IALLTOALLV_INTER_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Ialltoallv_allcomm_sched_auto(sendbuf, sendcounts, sdispls, sendtype,
                                                       recvbuf, recvcounts, rdispls, recvtype,
                                                       comm_ptr, is_persistent, sched_p,
                                                       sched_type_p);
                break;

            default:
                MPIR_Assert(0);
            /* *INDENT-ON* */
        }
    }

    MPIR_ERR_CHECK(mpi_errno);
    goto fn_exit;

  fallback:
    mpi_errno = MPIR_Ialltoallv_allcomm_sched_auto(sendbuf, sendcounts, sdispls, sendtype,
                                                   recvbuf, recvcounts, rdispls, recvtype,
                                                   comm_ptr, is_persistent, sched_p, sched_type_p);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Ialltoallv_impl(const void *sendbuf, const MPI_Aint sendcounts[], const MPI_Aint sdispls[],
                         MPI_Datatype sendtype, void *recvbuf, const MPI_Aint recvcounts[],
                         const MPI_Aint rdispls[], MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                         MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    *request = NULL;

    enum MPIR_sched_type sched_type;
    void *sched;
    mpi_errno = MPIR_Ialltoallv_sched_impl(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                                           recvcounts, rdispls, recvtype, comm_ptr, false, &sched,
                                           &sched_type);
    MPIR_ERR_CHECK(mpi_errno);

    MPII_SCHED_START(sched_type, sched, comm_ptr, request);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Ialltoallv(const void *sendbuf, const MPI_Aint sendcounts[], const MPI_Aint sdispls[],
                    MPI_Datatype sendtype, void *recvbuf, const MPI_Aint recvcounts[],
                    const MPI_Aint rdispls[], MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
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
