/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* Local Reduce remote send
 *
 * Remote group does a local intracommunicator reduce to rank 0. Rank
 * 0 then sends data to root.
 *
 * Cost: (lgp+1).alpha + n.(lgp+1).beta
 */

int MPIR_Reduce_inter_local_reduce_remote_send(const void *sendbuf,
                                               void *recvbuf,
                                               MPI_Aint count,
                                               MPI_Datatype datatype,
                                               MPI_Op op,
                                               int root,
                                               MPIR_Comm * comm_ptr, int coll_group,
                                               MPIR_Errflag_t errflag)
{
    int rank, mpi_errno;
    MPI_Status status;
    MPI_Aint true_extent, true_lb, extent;
    void *tmp_buf = NULL;
    MPIR_Comm *newcomm_ptr = NULL;
    MPIR_CHKLMEM_DECL(1);

    if (root == MPI_PROC_NULL) {
        /* local processes other than root do nothing */
        return MPI_SUCCESS;
    }

    if (root == MPI_ROOT) {
        /* root receives data from rank 0 on remote group */
        mpi_errno = MPIC_Recv(recvbuf, count, datatype, 0, MPIR_REDUCE_TAG,
                              comm_ptr, coll_group, &status);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        /* remote group. Rank 0 allocates temporary buffer, does
         * local intracommunicator reduce, and then sends the data
         * to root. */

        rank = comm_ptr->rank;

        if (rank == 0) {
            MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

            MPIR_Datatype_get_extent_macro(datatype, extent);
            MPIR_CHKLMEM_MALLOC(tmp_buf, void *, count * (MPL_MAX(extent, true_extent)), mpi_errno,
                                "temporary buffer", MPL_MEM_BUFFER);
            /* adjust for potential negative lower bound in datatype */
            tmp_buf = (void *) ((char *) tmp_buf - true_lb);
        }

        /* Get the local intracommunicator */
        if (!comm_ptr->local_comm) {
            mpi_errno = MPII_Setup_intercomm_localcomm(comm_ptr);
            MPIR_ERR_CHECK(mpi_errno);
        }

        newcomm_ptr = comm_ptr->local_comm;

        /* now do a local reduce on this intracommunicator */
        mpi_errno = MPIR_Reduce(sendbuf, tmp_buf, count, datatype, op, 0, newcomm_ptr,
                                coll_group, errflag);
        MPIR_ERR_CHECK(mpi_errno);

        if (rank == 0) {
            mpi_errno = MPIC_Send(tmp_buf, count, datatype, root,
                                  MPIR_REDUCE_TAG, comm_ptr, coll_group, errflag);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
