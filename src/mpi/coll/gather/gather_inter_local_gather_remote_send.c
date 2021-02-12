/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* Local gather remote send
 *
 * Remote group does a local intracommunicator gather to rank 0. Rank
 * 0 then sends data to root.
 *
 * Cost: (lgp+1).alpha + n.((p-1)/p).beta + n.beta
 */

int MPIR_Gather_inter_local_gather_remote_send(const void *sendbuf, MPI_Aint sendcount,
                                               MPI_Datatype sendtype, void *recvbuf,
                                               MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                                               MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
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
        /* root receives data from rank 0 on remote group */
        mpi_errno =
            MPIC_Recv(recvbuf, recvcount * remote_size, recvtype, 0, MPIR_GATHER_TAG, comm_ptr,
                      &status, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    } else {
        /* remote group. Rank 0 allocates temporary buffer, does
         * local intracommunicator gather, and then sends the data
         * to root. */
        MPI_Aint sendtype_sz;
        void *tmp_buf = NULL;

        rank = comm_ptr->rank;

        if (rank == 0) {
            MPIR_Datatype_get_size_macro(sendtype, sendtype_sz);
            MPIR_CHKLMEM_MALLOC(tmp_buf, void *,
                                sendcount * local_size * sendtype_sz, mpi_errno,
                                "tmp_buf", MPL_MEM_BUFFER);
        } else {
            /* silence -Wmaybe-uninitialized due to MPIR_Gather by non-zero ranks */
            sendtype_sz = 0;
        }

        /* all processes in remote group form new intracommunicator */
        if (!comm_ptr->local_comm) {
            mpi_errno = MPII_Setup_intercomm_localcomm(comm_ptr);
            MPIR_ERR_CHECK(mpi_errno);
        }

        newcomm_ptr = comm_ptr->local_comm;

        /* now do the a local gather on this intracommunicator */
        mpi_errno = MPIR_Gather(sendbuf, sendcount, sendtype,
                                tmp_buf, sendcount * sendtype_sz, MPI_BYTE, 0, newcomm_ptr,
                                errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }

        if (rank == 0) {
            mpi_errno = MPIC_Send(tmp_buf, sendcount * local_size * sendtype_sz, MPI_BYTE,
                                  root, MPIR_GATHER_TAG, comm_ptr, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag =
                    MPIX_ERR_PROC_FAILED ==
                    MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
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
