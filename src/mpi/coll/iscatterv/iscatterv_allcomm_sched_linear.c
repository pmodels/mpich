/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* Algorithm: Linear Scatterv
 *
 * Since the array of sendcounts is valid only on the root, we cannot do a tree
 * algorithm without first communicating the sendcounts to other processes.
 * Therefore, we simply use a linear algorithm for the scatter, which takes
 * (p-1) steps versus lgp steps for the tree algorithm. The bandwidth
 * requirement is the same for both algorithms.
 *
 * Cost = (p-1).alpha + n.((p-1)/p).beta
*/
int MPIR_Iscatterv_allcomm_sched_linear(const void *sendbuf, const MPI_Aint sendcounts[],
                                        const MPI_Aint displs[], MPI_Datatype sendtype,
                                        void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                        int root, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int rank, comm_size;
    MPI_Aint extent;
    int i;

    rank = comm_ptr->rank;

    /* If I'm the root, then scatter */
    if (((comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) && (root == rank)) ||
        ((comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) && (root == MPI_ROOT))) {
        if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM)
            comm_size = comm_ptr->local_size;
        else
            comm_size = comm_ptr->remote_size;

        MPIR_Datatype_get_extent_macro(sendtype, extent);

        for (i = 0; i < comm_size; i++) {
            if (sendcounts[i]) {
                if ((comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) && (i == rank)) {
                    if (recvbuf != MPI_IN_PLACE) {
                        mpi_errno = MPIR_Sched_copy(((char *) sendbuf + displs[rank] * extent),
                                                    sendcounts[rank], sendtype,
                                                    recvbuf, recvcount, recvtype, s);
                        MPIR_ERR_CHECK(mpi_errno);
                    }
                } else {
                    mpi_errno = MPIR_Sched_send(((char *) sendbuf + displs[i] * extent),
                                                sendcounts[i], sendtype, i, comm_ptr, s);
                    MPIR_ERR_CHECK(mpi_errno);
                }
            }
        }
    }

    else if (root != MPI_PROC_NULL) {
        /* non-root nodes, and in the intercomm. case, non-root nodes on remote side */
        if (recvcount) {
            mpi_errno = MPIR_Sched_recv(recvbuf, recvcount, recvtype, root, comm_ptr, s);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
