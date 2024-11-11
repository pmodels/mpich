/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
 * Linear
 *
 * Root receives from all processes, everyone else sends to root.
 */

int MPIR_Igatherv_allcomm_sched_linear(const void *sendbuf, MPI_Aint sendcount,
                                       MPI_Datatype sendtype, void *recvbuf,
                                       const MPI_Aint recvcounts[], const MPI_Aint displs[],
                                       MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                                       int coll_group, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    int comm_size, rank;
    MPI_Aint extent;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        MPIR_COLL_RANK_SIZE(comm_ptr, coll_group, rank, comm_size);
    } else {
        MPIR_Assert(coll_group == MPIR_SUBGROUP_NONE);
        rank = comm_ptr->rank;
        comm_size = comm_ptr->remote_size;
    }

    /* If rank == root, then I recv lots, otherwise I send */
    if (((comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) && (root == rank)) ||
        ((comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) && (root == MPI_ROOT))) {

        MPIR_Datatype_get_extent_macro(recvtype, extent);

        for (i = 0; i < comm_size; i++) {
            if (recvcounts[i]) {
                if ((comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) && (i == rank)) {
                    if (sendbuf != MPI_IN_PLACE) {
                        mpi_errno = MPIR_Sched_copy(sendbuf, sendcount, sendtype,
                                                    ((char *) recvbuf + displs[rank] * extent),
                                                    recvcounts[rank], recvtype, s);
                        MPIR_ERR_CHECK(mpi_errno);
                    }
                } else {
                    mpi_errno = MPIR_Sched_recv(((char *) recvbuf + displs[i] * extent),
                                                recvcounts[i], recvtype, i, comm_ptr, coll_group,
                                                s);
                    MPIR_ERR_CHECK(mpi_errno);
                }
            }
        }
    } else if (root != MPI_PROC_NULL) {
        /* non-root nodes, and in the intercomm. case, non-root nodes on remote side */
        if (sendcount) {
            mpi_errno =
                MPIR_Sched_send(sendbuf, sendcount, sendtype, root, comm_ptr, coll_group, s);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
