/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
*/

int MPIR_Alltoall_allcomm_auto(const void *sendbuf,
                               MPI_Aint sendcount,
                               MPI_Datatype sendtype,
                               void *recvbuf,
                               MPI_Aint recvcount,
                               MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                               MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__ALLTOALL,
        .comm_ptr = comm_ptr,

        .u.alltoall.sendbuf = sendbuf,
        .u.alltoall.sendcount = sendcount,
        .u.alltoall.sendtype = sendtype,
        .u.alltoall.recvcount = recvcount,
        .u.alltoall.recvbuf = recvbuf,
        .u.alltoall.recvtype = recvtype,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoall_intra_brucks:
            mpi_errno =
                MPIR_Alltoall_intra_brucks(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                           recvtype, comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoall_intra_pairwise:
            mpi_errno =
                MPIR_Alltoall_intra_pairwise(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                             recvtype, comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoall_intra_pairwise_sendrecv_replace:
            mpi_errno =
                MPIR_Alltoall_intra_pairwise_sendrecv_replace(sendbuf, sendcount, sendtype, recvbuf,
                                                              recvcount, recvtype, comm_ptr,
                                                              errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoall_intra_scattered:
            mpi_errno =
                MPIR_Alltoall_intra_scattered(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                              recvtype, comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoall_inter_pairwise_exchange:
            mpi_errno =
                MPIR_Alltoall_inter_pairwise_exchange(sendbuf, sendcount, sendtype, recvbuf,
                                                      recvcount, recvtype, comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoall_allcomm_nb:
            mpi_errno =
                MPIR_Alltoall_allcomm_nb(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                         comm_ptr, errflag);
            break;

        default:
            MPIR_Assert(0);
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Alltoall_impl(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                       void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                       MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        switch (MPIR_CVAR_ALLTOALL_INTRA_ALGORITHM) {
            case MPIR_CVAR_ALLTOALL_INTRA_ALGORITHM_brucks:
                mpi_errno = MPIR_Alltoall_intra_brucks(sendbuf, sendcount, sendtype,
                                                       recvbuf, recvcount, recvtype,
                                                       comm_ptr, errflag);
                break;
            case MPIR_CVAR_ALLTOALL_INTRA_ALGORITHM_pairwise:
                mpi_errno = MPIR_Alltoall_intra_pairwise(sendbuf, sendcount, sendtype,
                                                         recvbuf, recvcount, recvtype,
                                                         comm_ptr, errflag);
                break;
            case MPIR_CVAR_ALLTOALL_INTRA_ALGORITHM_pairwise_sendrecv_replace:
                mpi_errno = MPIR_Alltoall_intra_pairwise_sendrecv_replace(sendbuf, sendcount,
                                                                          sendtype, recvbuf,
                                                                          recvcount, recvtype,
                                                                          comm_ptr, errflag);
                break;
            case MPIR_CVAR_ALLTOALL_INTRA_ALGORITHM_scattered:
                mpi_errno = MPIR_Alltoall_intra_scattered(sendbuf, sendcount, sendtype,
                                                          recvbuf, recvcount, recvtype,
                                                          comm_ptr, errflag);
                break;
            case MPIR_CVAR_ALLTOALL_INTRA_ALGORITHM_nb:
                mpi_errno = MPIR_Alltoall_allcomm_nb(sendbuf, sendcount, sendtype,
                                                     recvbuf, recvcount, recvtype, comm_ptr,
                                                     errflag);
                break;
            case MPIR_CVAR_ALLTOALL_INTRA_ALGORITHM_auto:
                mpi_errno = MPIR_Alltoall_allcomm_auto(sendbuf, sendcount, sendtype,
                                                       recvbuf, recvcount, recvtype, comm_ptr,
                                                       errflag);
                break;
            default:
                MPIR_Assert(0);
        }
    } else {
        /* intercommunicator */
        switch (MPIR_CVAR_ALLTOALL_INTER_ALGORITHM) {
            case MPIR_CVAR_ALLTOALL_INTER_ALGORITHM_pairwise_exchange:
                mpi_errno = MPIR_Alltoall_inter_pairwise_exchange(sendbuf, sendcount, sendtype,
                                                                  recvbuf, recvcount, recvtype,
                                                                  comm_ptr, errflag);
                break;
            case MPIR_CVAR_ALLTOALL_INTER_ALGORITHM_nb:
                mpi_errno = MPIR_Alltoall_allcomm_nb(sendbuf, sendcount, sendtype,
                                                     recvbuf, recvcount, recvtype, comm_ptr,
                                                     errflag);
                break;
            case MPIR_CVAR_ALLTOALL_INTER_ALGORITHM_auto:
                mpi_errno = MPIR_Alltoall_allcomm_auto(sendbuf, sendcount, sendtype,
                                                       recvbuf, recvcount, recvtype, comm_ptr,
                                                       errflag);
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

int MPIR_Alltoall(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                  void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                  MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_ALLTOALL_DEVICE_COLLECTIVE)) {
        mpi_errno =
            MPID_Alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm_ptr,
                          errflag);
    } else {
        mpi_errno = MPIR_Alltoall_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                       comm_ptr, errflag);
    }

    return mpi_errno;
}
