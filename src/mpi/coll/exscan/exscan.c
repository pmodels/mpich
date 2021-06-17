/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
*/


int MPIR_Exscan_allcomm_auto(const void *sendbuf, void *recvbuf, MPI_Aint count,
                             MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                             MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__EXSCAN,
        .comm_ptr = comm_ptr,

        .u.exscan.sendbuf = sendbuf,
        .u.exscan.recvbuf = recvbuf,
        .u.exscan.count = count,
        .u.exscan.datatype = datatype,
        .u.exscan.op = op,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Exscan_intra_recursive_doubling:
            mpi_errno =
                MPIR_Exscan_intra_recursive_doubling(sendbuf, recvbuf, count, datatype, op,
                                                     comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Exscan_allcomm_nb:
            mpi_errno =
                MPIR_Exscan_allcomm_nb(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
            break;

        default:
            MPIR_Assert(0);
    }

    return mpi_errno;
}

int MPIR_Exscan_impl(const void *sendbuf, void *recvbuf, MPI_Aint count,
                     MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                     MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    switch (MPIR_CVAR_EXSCAN_INTRA_ALGORITHM) {
        case MPIR_CVAR_EXSCAN_INTRA_ALGORITHM_recursive_doubling:
            mpi_errno =
                MPIR_Exscan_intra_recursive_doubling(sendbuf, recvbuf, count, datatype, op,
                                                     comm_ptr, errflag);
            break;
        case MPIR_CVAR_EXSCAN_INTRA_ALGORITHM_nb:
            mpi_errno =
                MPIR_Exscan_allcomm_nb(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
            break;
        case MPIR_CVAR_EXSCAN_INTRA_ALGORITHM_auto:
            mpi_errno =
                MPIR_Exscan_allcomm_auto(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
            break;
        default:
            MPIR_Assert(0);
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**coll_fail");
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Exscan(const void *sendbuf, void *recvbuf, MPI_Aint count,
                MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    void *in_recvbuf = recvbuf;
    void *host_sendbuf;
    void *host_recvbuf;

    MPIR_Coll_host_buffer_alloc(sendbuf, recvbuf, count, datatype, &host_sendbuf, &host_recvbuf);
    if (host_sendbuf)
        sendbuf = host_sendbuf;
    if (host_recvbuf)
        recvbuf = host_recvbuf;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_EXSCAN_DEVICE_COLLECTIVE)) {
        mpi_errno = MPID_Exscan(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
    } else {
        mpi_errno = MPIR_Exscan_impl(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
    }

    /* Copy out data from host recv buffer to GPU buffer */
    if (host_recvbuf) {
        recvbuf = in_recvbuf;
        MPIR_Localcopy(host_recvbuf, count, datatype, recvbuf, count, datatype);
    }

    MPIR_Coll_host_buffer_free(host_sendbuf, host_recvbuf);

    return mpi_errno;
}
