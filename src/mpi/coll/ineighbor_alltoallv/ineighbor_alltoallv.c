/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
/* for MPIR_TSP_sched_t */
#include "tsp_gentran.h"
#include "gentran_utils.h"
#include "../ineighbor_alltoallv/ineighbor_alltoallv_tsp_linear_algos_prototypes.h"

/*
*/

int MPIR_Ineighbor_alltoallv_allcomm_sched_auto(const void *sendbuf, const MPI_Aint sendcounts[],
                                                const MPI_Aint sdispls[], MPI_Datatype sendtype,
                                                void *recvbuf, const MPI_Aint recvcounts[],
                                                const MPI_Aint rdispls[], MPI_Datatype recvtype,
                                                MPIR_Comm * comm_ptr, bool is_persistent,
                                                void **sched_p, enum MPIR_sched_type *sched_type_p)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__INEIGHBOR_ALLTOALLV,
        .comm_ptr = comm_ptr,

        .u.ineighbor_alltoallv.sendbuf = sendbuf,
        .u.ineighbor_alltoallv.sendcounts = sendcounts,
        .u.ineighbor_alltoallv.sdispls = sdispls,
        .u.ineighbor_alltoallv.sendtype = sendtype,
        .u.ineighbor_alltoallv.recvbuf = recvbuf,
        .u.ineighbor_alltoallv.recvcounts = recvcounts,
        .u.ineighbor_alltoallv.rdispls = rdispls,
        .u.ineighbor_alltoallv.recvtype = recvtype,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        /* *INDENT-OFF* */
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ineighbor_alltoallv_allcomm_gentran_linear:
            MPII_GENTRAN_CREATE_SCHED_P();
            mpi_errno =
                MPIR_TSP_Ineighbor_alltoallv_sched_allcomm_linear(sendbuf, sendcounts, sdispls,
                                                                  sendtype, recvbuf, recvcounts,
                                                                  rdispls, recvtype, comm_ptr,
                                                                  *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ineighbor_alltoallv_intra_sched_auto:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno =
                MPIR_Ineighbor_alltoallv_intra_sched_auto(sendbuf, sendcounts, sdispls, sendtype,
                                                          recvbuf, recvcounts, rdispls, recvtype,
                                                          comm_ptr, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ineighbor_alltoallv_inter_sched_auto:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno =
                MPIR_Ineighbor_alltoallv_inter_sched_auto(sendbuf, sendcounts, sdispls, sendtype,
                                                          recvbuf, recvcounts, rdispls, recvtype,
                                                          comm_ptr, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ineighbor_alltoallv_allcomm_sched_linear:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno =
                MPIR_Ineighbor_alltoallv_allcomm_sched_linear(sendbuf, sendcounts, sdispls,
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

int MPIR_Ineighbor_alltoallv_intra_sched_auto(const void *sendbuf, const MPI_Aint sendcounts[],
                                              const MPI_Aint sdispls[], MPI_Datatype sendtype,
                                              void *recvbuf, const MPI_Aint recvcounts[],
                                              const MPI_Aint rdispls[], MPI_Datatype recvtype,
                                              MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_Ineighbor_alltoallv_allcomm_sched_linear(sendbuf, sendcounts, sdispls, sendtype,
                                                      recvbuf, recvcounts, rdispls, recvtype,
                                                      comm_ptr, s);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPIR_Ineighbor_alltoallv_inter_sched_auto(const void *sendbuf, const MPI_Aint sendcounts[],
                                              const MPI_Aint sdispls[], MPI_Datatype sendtype,
                                              void *recvbuf, const MPI_Aint recvcounts[],
                                              const MPI_Aint rdispls[], MPI_Datatype recvtype,
                                              MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_Ineighbor_alltoallv_allcomm_sched_linear(sendbuf, sendcounts, sdispls, sendtype,
                                                      recvbuf, recvcounts, rdispls, recvtype,
                                                      comm_ptr, s);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPIR_Ineighbor_alltoallv_sched_auto(const void *sendbuf, const MPI_Aint sendcounts[],
                                        const MPI_Aint sdispls[], MPI_Datatype sendtype,
                                        void *recvbuf, const MPI_Aint recvcounts[],
                                        const MPI_Aint rdispls[], MPI_Datatype recvtype,
                                        MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        mpi_errno =
            MPIR_Ineighbor_alltoallv_intra_sched_auto(sendbuf, sendcounts, sdispls,
                                                      sendtype, recvbuf, recvcounts,
                                                      rdispls, recvtype, comm_ptr, s);
    } else {
        mpi_errno =
            MPIR_Ineighbor_alltoallv_inter_sched_auto(sendbuf, sendcounts, sdispls,
                                                      sendtype, recvbuf, recvcounts,
                                                      rdispls, recvtype, comm_ptr, s);
    }

    return mpi_errno;
}

int MPIR_Ineighbor_alltoallv_sched_impl(const void *sendbuf, const MPI_Aint sendcounts[],
                                        const MPI_Aint sdispls[], MPI_Datatype sendtype,
                                        void *recvbuf, const MPI_Aint recvcounts[],
                                        const MPI_Aint rdispls[], MPI_Datatype recvtype,
                                        MPIR_Comm * comm_ptr, bool is_persistent, void **sched_p,
                                        enum MPIR_sched_type *sched_type_p)
{
    int mpi_errno = MPI_SUCCESS;

    /* If the user picks one of the transport-enabled algorithms, branch there
     * before going down to the MPIR_Sched-based algorithms. */
    /* TODO - Eventually the intention is to replace all of the
     * MPIR_Sched-based algorithms with transport-enabled algorithms, but that
     * will require sufficient performance testing and replacement algorithms. */
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        switch (MPIR_CVAR_INEIGHBOR_ALLTOALLV_INTRA_ALGORITHM) {
            /* *INDENT-OFF* */
            case MPIR_CVAR_INEIGHBOR_ALLTOALLV_INTRA_ALGORITHM_gentran_linear:
                MPII_GENTRAN_CREATE_SCHED_P();
                mpi_errno =
                    MPIR_TSP_Ineighbor_alltoallv_sched_allcomm_linear(sendbuf, sendcounts, sdispls,
                                                                      sendtype, recvbuf, recvcounts,
                                                                      rdispls, recvtype, comm_ptr,
                                                                      *sched_p);
                break;

            case MPIR_CVAR_INEIGHBOR_ALLTOALLV_INTRA_ALGORITHM_sched_linear:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno =
                    MPIR_Ineighbor_alltoallv_allcomm_sched_linear(sendbuf, sendcounts, sdispls,
                                                                  sendtype, recvbuf, recvcounts,
                                                                  rdispls, recvtype, comm_ptr,
                                                                  *sched_p);
                break;

            case MPIR_CVAR_INEIGHBOR_ALLTOALLV_INTRA_ALGORITHM_sched_auto:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ineighbor_alltoallv_intra_sched_auto(sendbuf, sendcounts, sdispls,
                                                                      sendtype, recvbuf, recvcounts,
                                                                      rdispls, recvtype, comm_ptr,
                                                                      *sched_p);
                break;

            case MPIR_CVAR_INEIGHBOR_ALLTOALLV_INTRA_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Ineighbor_alltoallv_allcomm_sched_auto(sendbuf, sendcounts, sdispls,
                                                                sendtype, recvbuf, recvcounts,
                                                                rdispls, recvtype, comm_ptr,
                                                                is_persistent, sched_p,
                                                                sched_type_p);
                break;

            default:
                MPIR_Assert(0);
            /* *INDENT-ON* */
        }
    } else {
        switch (MPIR_CVAR_INEIGHBOR_ALLTOALLV_INTER_ALGORITHM) {
            /* *INDENT-OFF* */
            case MPIR_CVAR_INEIGHBOR_ALLTOALLV_INTER_ALGORITHM_gentran_linear:
                MPII_GENTRAN_CREATE_SCHED_P();
                mpi_errno =
                    MPIR_TSP_Ineighbor_alltoallv_sched_allcomm_linear(sendbuf, sendcounts, sdispls,
                                                                      sendtype, recvbuf, recvcounts,
                                                                      rdispls, recvtype, comm_ptr,
                                                                      *sched_p);
                break;

            case MPIR_CVAR_INEIGHBOR_ALLTOALLV_INTER_ALGORITHM_sched_linear:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno =
                    MPIR_Ineighbor_alltoallv_allcomm_sched_linear(sendbuf, sendcounts, sdispls,
                                                                  sendtype, recvbuf, recvcounts,
                                                                  rdispls, recvtype, comm_ptr,
                                                                  *sched_p);
                break;

            case MPIR_CVAR_INEIGHBOR_ALLTOALLV_INTER_ALGORITHM_sched_auto:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno =
                    MPIR_Ineighbor_alltoallv_inter_sched_auto(sendbuf, sendcounts, sdispls,
                                                              sendtype, recvbuf, recvcounts,
                                                              rdispls, recvtype, comm_ptr,
                                                              *sched_p);
                break;

            case MPIR_CVAR_INEIGHBOR_ALLTOALLV_INTER_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Ineighbor_alltoallv_allcomm_sched_auto(sendbuf, sendcounts, sdispls,
                                                                sendtype, recvbuf, recvcounts,
                                                                rdispls, recvtype, comm_ptr,
                                                                is_persistent, sched_p,
                                                                sched_type_p);
                break;

            default:
                MPIR_Assert(0);
            /* *INDENT-ON* */
        }
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Ineighbor_alltoallv_impl(const void *sendbuf, const MPI_Aint sendcounts[],
                                  const MPI_Aint sdispls[], MPI_Datatype sendtype,
                                  void *recvbuf, const MPI_Aint recvcounts[],
                                  const MPI_Aint rdispls[], MPI_Datatype recvtype,
                                  MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    *request = NULL;

    enum MPIR_sched_type sched_type;
    void *sched;
    mpi_errno = MPIR_Ineighbor_alltoallv_sched_impl(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                                                    recvcounts, rdispls, recvtype, comm_ptr, false,
                                                    &sched, &sched_type);
    MPIR_ERR_CHECK(mpi_errno);

    MPII_SCHED_START(sched_type, sched, comm_ptr, request);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Ineighbor_alltoallv(const void *sendbuf, const MPI_Aint sendcounts[],
                             const MPI_Aint sdispls[], MPI_Datatype sendtype,
                             void *recvbuf, const MPI_Aint recvcounts[],
                             const MPI_Aint rdispls[], MPI_Datatype recvtype,
                             MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_BARRIER_DEVICE_COLLECTIVE)) {
        mpi_errno =
            MPID_Ineighbor_alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts,
                                     rdispls, recvtype, comm_ptr, request);
    } else {
        mpi_errno = MPIR_Ineighbor_alltoallv_impl(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                                                  recvcounts, rdispls, recvtype, comm_ptr, request);
    }

    return mpi_errno;
}
