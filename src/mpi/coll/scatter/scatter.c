/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
*/


int MPIR_Scatter_allcomm_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                              void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                              MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__SCATTER,
        .comm_ptr = comm_ptr,

        .u.scatter.sendbuf = sendbuf,
        .u.scatter.sendcount = sendcount,
        .u.scatter.sendtype = sendtype,
        .u.scatter.recvcount = recvcount,
        .u.scatter.recvbuf = recvbuf,
        .u.scatter.recvtype = recvtype,
        .u.scatter.root = root,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Scatter_intra_binomial:
            mpi_errno =
                MPIR_Scatter_intra_binomial(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                            recvtype, root, comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Scatter_inter_linear:
            mpi_errno =
                MPIR_Scatter_inter_linear(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                          recvtype, root, comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Scatter_inter_remote_send_local_scatter:
            mpi_errno =
                MPIR_Scatter_inter_remote_send_local_scatter(sendbuf, sendcount, sendtype, recvbuf,
                                                             recvcount, recvtype, root, comm_ptr,
                                                             errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Scatter_allcomm_nb:
            mpi_errno =
                MPIR_Scatter_allcomm_nb(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                        root, comm_ptr, errflag);
            break;

        default:
            MPIR_Assert(0);
    }
    return mpi_errno;
}

int MPIR_Scatter_impl(const void *sendbuf, MPI_Aint sendcount,
                      MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                      MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                      MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        switch (MPIR_CVAR_SCATTER_INTRA_ALGORITHM) {
            case MPIR_CVAR_SCATTER_INTRA_ALGORITHM_binomial:
                mpi_errno = MPIR_Scatter_intra_binomial(sendbuf, sendcount, sendtype,
                                                        recvbuf, recvcount, recvtype, root,
                                                        comm_ptr, errflag);
                break;
            case MPIR_CVAR_SCATTER_INTRA_ALGORITHM_nb:
                mpi_errno = MPIR_Scatter_allcomm_nb(sendbuf, sendcount, sendtype,
                                                    recvbuf, recvcount, recvtype, root, comm_ptr,
                                                    errflag);
                break;
            case MPIR_CVAR_SCATTER_INTRA_ALGORITHM_auto:
                mpi_errno = MPIR_Scatter_allcomm_auto(sendbuf, sendcount, sendtype,
                                                      recvbuf, recvcount, recvtype, root,
                                                      comm_ptr, errflag);
                break;
            default:
                MPIR_Assert(0);
        }
    } else {
        /* intercommunicator */
        switch (MPIR_CVAR_SCATTER_INTER_ALGORITHM) {
            case MPIR_CVAR_SCATTER_INTER_ALGORITHM_linear:
                mpi_errno = MPIR_Scatter_inter_linear(sendbuf, sendcount, sendtype,
                                                      recvbuf, recvcount, recvtype, root,
                                                      comm_ptr, errflag);
                break;
            case MPIR_CVAR_SCATTER_INTER_ALGORITHM_nb:
                mpi_errno = MPIR_Scatter_allcomm_nb(sendbuf, sendcount, sendtype,
                                                    recvbuf, recvcount, recvtype, root, comm_ptr,
                                                    errflag);
                break;
            case MPIR_CVAR_SCATTER_INTER_ALGORITHM_remote_send_local_scatter:
                mpi_errno =
                    MPIR_Scatter_inter_remote_send_local_scatter(sendbuf, sendcount, sendtype,
                                                                 recvbuf, recvcount, recvtype, root,
                                                                 comm_ptr, errflag);
                break;
            case MPIR_CVAR_SCATTER_INTER_ALGORITHM_auto:
                mpi_errno = MPIR_Scatter_allcomm_auto(sendbuf, sendcount, sendtype,
                                                      recvbuf, recvcount, recvtype, root,
                                                      comm_ptr, errflag);
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

int MPIR_Scatter(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                 void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                 MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_SCATTER_DEVICE_COLLECTIVE)) {
        mpi_errno =
            MPID_Scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm_ptr,
                         errflag);
    } else {
        mpi_errno = MPIR_Scatter_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                      root, comm_ptr, errflag);
    }

    return mpi_errno;
}
