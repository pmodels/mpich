/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* Remote send local scatter
 *
 * Root sends to rank 0 in remote group.  rank 0 does local
 * intracommunicator scatter (binomial tree).
 *
 * Cost: (lgp+1).alpha + n.((p-1)/p).beta + n.beta
 */

int MPIR_Scatter_inter_remote_send_local_scatter(const void *sendbuf, MPI_Aint sendcount,
                                                 MPI_Datatype sendtype, void *recvbuf,
                                                 MPI_Aint recvcount, MPI_Datatype recvtype,
                                                 int root, MPIR_Comm * comm_ptr,
                                                 MPIR_Errflag_t * errflag)
{
    int rank, local_size, remote_size, mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Status status;
    MPIR_Comm *newcomm_ptr = NULL;
    MPIR_CHKLMEM_DECL(1);

    if (root == MPI_PROC_NULL) {
        /* local processes other than root do nothing */
        return MPI_SUCCESS;
    }

    remote_size = comm_ptr->remote_size;
    local_size = comm_ptr->local_size;

    if (root == MPI_ROOT) {
        /* root sends all data to rank 0 on remote group and returns */
        mpi_errno =
            MPIC_Send(sendbuf, sendcount * remote_size, sendtype, 0, MPIR_SCATTER_TAG, comm_ptr,
                      errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
        goto fn_exit;
    } else {
        /* remote group. rank 0 receives data from root. need to
         * allocate temporary buffer to store this data. */
        MPI_Aint recvtype_sz;
        void *tmp_buf = NULL;

        rank = comm_ptr->rank;

        if (rank == 0) {
            MPIR_Datatype_get_size_macro(recvtype, recvtype_sz);
            MPIR_CHKLMEM_MALLOC(tmp_buf, void *,
                                recvcount * local_size * recvtype_sz, mpi_errno,
                                "tmp_buf", MPL_MEM_BUFFER);

            mpi_errno = MPIC_Recv(tmp_buf, recvcount * local_size * recvtype_sz, MPI_BYTE,
                                  root, MPIR_SCATTER_TAG, comm_ptr, &status, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag =
                    MPIX_ERR_PROC_FAILED ==
                    MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        } else {
            /* silience -Wmaybe-uninitialized due to MPIR_Scatter by non-zero ranks */
            recvtype_sz = 0;
        }

        /* Get the local intracommunicator */
        if (!comm_ptr->local_comm)
            MPII_Setup_intercomm_localcomm(comm_ptr);

        newcomm_ptr = comm_ptr->local_comm;

        /* now do the usual scatter on this intracommunicator */
        mpi_errno = MPIR_Scatter(tmp_buf, recvcount * recvtype_sz, MPI_BYTE,
                                 recvbuf, recvcount, recvtype, 0, newcomm_ptr, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}
