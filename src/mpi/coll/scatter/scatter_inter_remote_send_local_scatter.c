/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* Remote send local scatter
 *
 * Root sends to rank 0 in remote group.  rank 0 does local
 * intracommunicator scatter (binomial tree).
 *
 * Cost: (lgp+1).alpha + n.((p-1)/p).beta + n.beta
 */

#undef FUNCNAME
#define FUNCNAME MPIR_Scatter_inter_remote_send_local_scatter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Scatter_inter_remote_send_local_scatter(const void *sendbuf, int sendcount,
                                                 MPI_Datatype sendtype, void *recvbuf,
                                                 int recvcount, MPI_Datatype recvtype, int root,
                                                 MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int rank, local_size, remote_size, mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Status status;
    MPI_Aint extent, true_extent, true_lb = 0;
    void *tmp_buf = NULL;
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

        rank = comm_ptr->rank;

        if (rank == 0) {
            MPIR_Type_get_true_extent_impl(recvtype, &true_lb, &true_extent);

            MPIR_Datatype_get_extent_macro(recvtype, extent);
            MPIR_Ensure_Aint_fits_in_pointer(extent * recvcount * local_size);
            MPIR_Ensure_Aint_fits_in_pointer(MPIR_VOID_PTR_CAST_TO_MPI_AINT sendbuf +
                                             sendcount * remote_size * extent);

            MPIR_CHKLMEM_MALLOC(tmp_buf, void *,
                                recvcount * local_size * (MPL_MAX(extent, true_extent)), mpi_errno,
                                "tmp_buf", MPL_MEM_BUFFER);

            /* adjust for potential negative lower bound in datatype */
            tmp_buf = (void *) ((char *) tmp_buf - true_lb);

            mpi_errno =
                MPIC_Recv(tmp_buf, recvcount * local_size, recvtype, root, MPIR_SCATTER_TAG,
                          comm_ptr, &status, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag =
                    MPIX_ERR_PROC_FAILED ==
                    MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }

        /* Get the local intracommunicator */
        if (!comm_ptr->local_comm)
            MPII_Setup_intercomm_localcomm(comm_ptr);

        newcomm_ptr = comm_ptr->local_comm;

        /* now do the usual scatter on this intracommunicator */
        mpi_errno =
            MPIR_Scatter(tmp_buf, recvcount, recvtype, recvbuf, recvcount, recvtype, 0, newcomm_ptr,
                         errflag);
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
