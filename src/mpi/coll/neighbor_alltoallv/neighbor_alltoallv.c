/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
*/

/* any non-MPI functions go here, especially non-static ones */


int MPIR_Neighbor_alltoallv_allcomm_auto(const void *sendbuf, const MPI_Aint sendcounts[],
                                         const MPI_Aint sdispls[], MPI_Datatype sendtype,
                                         void *recvbuf, const MPI_Aint recvcounts[],
                                         const MPI_Aint rdispls[], MPI_Datatype recvtype,
                                         MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__NEIGHBOR_ALLTOALLV,
        .comm_ptr = comm_ptr,

        .u.neighbor_alltoallv.sendbuf = sendbuf,
        .u.neighbor_alltoallv.sendcounts = sendcounts,
        .u.neighbor_alltoallv.sdispls = sdispls,
        .u.neighbor_alltoallv.sendtype = sendtype,
        .u.neighbor_alltoallv.recvbuf = recvbuf,
        .u.neighbor_alltoallv.recvcounts = recvcounts,
        .u.neighbor_alltoallv.rdispls = rdispls,
        .u.neighbor_alltoallv.recvtype = recvtype,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Neighbor_alltoallv_allcomm_nb:
            mpi_errno =
                MPIR_Neighbor_alltoallv_allcomm_nb(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                                                   recvcounts, rdispls, recvtype, comm_ptr);
            break;

        default:
            MPIR_Assert(0);
    }

    return mpi_errno;
}

int MPIR_Neighbor_alltoallv_impl(const void *sendbuf, const MPI_Aint sendcounts[],
                                 const MPI_Aint sdispls[], MPI_Datatype sendtype,
                                 void *recvbuf, const MPI_Aint recvcounts[],
                                 const MPI_Aint rdispls[], MPI_Datatype recvtype,
                                 MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        switch (MPIR_CVAR_NEIGHBOR_ALLTOALLV_INTRA_ALGORITHM) {
            case MPIR_CVAR_NEIGHBOR_ALLTOALLV_INTRA_ALGORITHM_nb:
                mpi_errno =
                    MPIR_Neighbor_alltoallv_allcomm_nb(sendbuf, sendcounts, sdispls, sendtype,
                                                       recvbuf, recvcounts, rdispls, recvtype,
                                                       comm_ptr);
                break;
            case MPIR_CVAR_NEIGHBOR_ALLTOALLV_INTRA_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Neighbor_alltoallv_allcomm_auto(sendbuf, sendcounts, sdispls, sendtype,
                                                         recvbuf, recvcounts, rdispls, recvtype,
                                                         comm_ptr);
                break;
            default:
                MPIR_Assert(0);
        }
    } else {
        switch (MPIR_CVAR_NEIGHBOR_ALLTOALLV_INTER_ALGORITHM) {
            case MPIR_CVAR_NEIGHBOR_ALLTOALLV_INTER_ALGORITHM_nb:
                mpi_errno =
                    MPIR_Neighbor_alltoallv_allcomm_nb(sendbuf, sendcounts, sdispls, sendtype,
                                                       recvbuf, recvcounts, rdispls, recvtype,
                                                       comm_ptr);
                break;
            case MPIR_CVAR_NEIGHBOR_ALLTOALLV_INTER_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Neighbor_alltoallv_allcomm_auto(sendbuf, sendcounts, sdispls, sendtype,
                                                         recvbuf, recvcounts, rdispls, recvtype,
                                                         comm_ptr);
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

int MPIR_Neighbor_alltoallv(const void *sendbuf, const MPI_Aint sendcounts[],
                            const MPI_Aint sdispls[], MPI_Datatype sendtype,
                            void *recvbuf, const MPI_Aint recvcounts[],
                            const MPI_Aint rdispls[], MPI_Datatype recvtype, MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_BARRIER_DEVICE_COLLECTIVE)) {
        mpi_errno =
            MPID_Neighbor_alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts,
                                    rdispls, recvtype, comm_ptr);
    } else {
        mpi_errno = MPIR_Neighbor_alltoallv_impl(sendbuf, sendcounts, sdispls,
                                                 sendtype, recvbuf, recvcounts,
                                                 rdispls, recvtype, comm_ptr);
    }

    return mpi_errno;
}
