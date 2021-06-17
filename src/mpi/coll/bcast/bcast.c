/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
*/


int MPIR_Bcast_allcomm_auto(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                            MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__BCAST,
        .comm_ptr = comm_ptr,

        .u.bcast.buffer = buffer,
        .u.bcast.count = count,
        .u.bcast.datatype = datatype,
        .u.bcast.root = root,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_intra_binomial:
            mpi_errno = MPIR_Bcast_intra_binomial(buffer, count, datatype, root, comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_intra_scatter_recursive_doubling_allgather:
            mpi_errno =
                MPIR_Bcast_intra_scatter_recursive_doubling_allgather(buffer, count, datatype, root,
                                                                      comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_intra_scatter_ring_allgather:
            mpi_errno =
                MPIR_Bcast_intra_scatter_ring_allgather(buffer, count, datatype, root, comm_ptr,
                                                        errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_intra_smp:
            mpi_errno = MPIR_Bcast_intra_smp(buffer, count, datatype, root, comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_inter_remote_send_local_bcast:
            mpi_errno =
                MPIR_Bcast_inter_remote_send_local_bcast(buffer, count, datatype, root, comm_ptr,
                                                         errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_allcomm_nb:
            mpi_errno = MPIR_Bcast_allcomm_nb(buffer, count, datatype, root, comm_ptr, errflag);
            break;

        default:
            MPIR_Assert(0);
    }

    return mpi_errno;
}

int MPIR_Bcast_impl(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                    MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        switch (MPIR_CVAR_BCAST_INTRA_ALGORITHM) {
            case MPIR_CVAR_BCAST_INTRA_ALGORITHM_binomial:
                mpi_errno =
                    MPIR_Bcast_intra_binomial(buffer, count, datatype, root, comm_ptr, errflag);
                break;
            case MPIR_CVAR_BCAST_INTRA_ALGORITHM_scatter_recursive_doubling_allgather:
                mpi_errno =
                    MPIR_Bcast_intra_scatter_recursive_doubling_allgather(buffer, count, datatype,
                                                                          root, comm_ptr, errflag);
                break;
            case MPIR_CVAR_BCAST_INTRA_ALGORITHM_scatter_ring_allgather:
                mpi_errno =
                    MPIR_Bcast_intra_scatter_ring_allgather(buffer, count, datatype, root, comm_ptr,
                                                            errflag);
                break;
            case MPIR_CVAR_BCAST_INTRA_ALGORITHM_nb:
                mpi_errno = MPIR_Bcast_allcomm_nb(buffer, count, datatype, root, comm_ptr, errflag);
                break;
            case MPIR_CVAR_BCAST_INTRA_ALGORITHM_smp:
                mpi_errno = MPIR_Bcast_intra_smp(buffer, count, datatype, root, comm_ptr, errflag);
                break;
            case MPIR_CVAR_BCAST_INTRA_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Bcast_allcomm_auto(buffer, count, datatype, root, comm_ptr, errflag);
                break;
            default:
                MPIR_Assert(0);
        }
    } else {
        /* intercommunicator */
        switch (MPIR_CVAR_BCAST_INTER_ALGORITHM) {
            case MPIR_CVAR_BCAST_INTER_ALGORITHM_remote_send_local_bcast:
                mpi_errno =
                    MPIR_Bcast_inter_remote_send_local_bcast(buffer, count, datatype, root,
                                                             comm_ptr, errflag);
                break;
            case MPIR_CVAR_BCAST_INTER_ALGORITHM_nb:
                mpi_errno = MPIR_Bcast_allcomm_nb(buffer, count, datatype, root, comm_ptr, errflag);
                break;
            case MPIR_CVAR_BCAST_INTER_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Bcast_allcomm_auto(buffer, count, datatype, root, comm_ptr, errflag);
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

int MPIR_Bcast(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root, MPIR_Comm * comm_ptr,
               MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_BCAST_DEVICE_COLLECTIVE)) {
        mpi_errno = MPID_Bcast(buffer, count, datatype, root, comm_ptr, errflag);
    } else {
        mpi_errno = MPIR_Bcast_impl(buffer, count, datatype, root, comm_ptr, errflag);
    }

    return mpi_errno;
}
