/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

int MPIR_Allreduce_intra_smp(const void *sendbuf, void *recvbuf, MPI_Aint count,
                             MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr, int coll_attr)
{
    int mpi_errno = MPI_SUCCESS;

    /* on each node, do a reduce to the local root */
    if (comm_ptr->node_comm != NULL) {
        /* take care of the MPI_IN_PLACE case. For reduce,
         * MPI_IN_PLACE is specified only on the root;
         * for allreduce it is specified on all processes. */

        if ((sendbuf == MPI_IN_PLACE) && (comm_ptr->node_comm->rank != 0)) {
            /* IN_PLACE and not root of reduce. Data supplied to this
             * allreduce is in recvbuf. Pass that as the sendbuf to reduce. */

            mpi_errno =
                MPIR_Reduce(recvbuf, NULL, count, datatype, op, 0, comm_ptr->node_comm, coll_attr);
            MPIR_ERR_CHECK(mpi_errno);
        } else {
            mpi_errno =
                MPIR_Reduce(sendbuf, recvbuf, count, datatype, op, 0, comm_ptr->node_comm,
                            coll_attr);
            MPIR_ERR_CHECK(mpi_errno);
        }
    } else {
        /* only one process on the node. copy sendbuf to recvbuf */
        if (sendbuf != MPI_IN_PLACE) {
            mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, recvbuf, count, datatype);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

    /* now do an IN_PLACE allreduce among the local roots of all nodes */
    if (comm_ptr->node_roots_comm != NULL) {
        mpi_errno =
            MPIR_Allreduce(MPI_IN_PLACE, recvbuf, count, datatype, op, comm_ptr->node_roots_comm,
                           coll_attr);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* now broadcast the result among local processes */
    if (comm_ptr->node_comm != NULL) {
        mpi_errno = MPIR_Bcast(recvbuf, count, datatype, 0, comm_ptr->node_comm, coll_attr);
        MPIR_ERR_CHECK(mpi_errno);
    }
    goto fn_exit;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
