/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

int MPIR_Allreduce_intra_smp(MPIR_Csel_coll_sig_s * coll_sig, MPII_Csel_container_s * cnt)
{
    MPIR_Comm *comm_ptr = coll_sig->comm_ptr;
    const void *sendbuf = coll_sig->u.allreduce.sendbuf;
    void *recvbuf = coll_sig->u.allreduce.recvbuf;
    MPI_Aint count = coll_sig->u.allreduce.count;
    MPI_Datatype datatype = coll_sig->u.allreduce.datatype;
    MPI_Op op = coll_sig->u.allreduce.op;
    int mpi_errno = MPI_SUCCESS;

    /* on each node, do a reduce to the local root */
    if (comm_ptr->node_comm != NULL) {
        /* take care of the MPI_IN_PLACE case. For reduce,
         * MPI_IN_PLACE is specified only on the root;
         * for allreduce it is specified on all processes. */

        if ((sendbuf == MPI_IN_PLACE) && (comm_ptr->node_comm->rank != 0)) {
            /* IN_PLACE and not root of reduce. Data supplied to this
             * allreduce is in recvbuf. Pass that as the sendbuf to reduce. */

            MPIR_COLL_INNER(Reduce, coll_sig, recvbuf, NULL, count, datatype, op, 0,
                            comm_ptr->node_comm);
        } else {
            MPIR_COLL_INNER(Reduce, coll_sig, sendbuf, recvbuf, count, datatype, op, 0,
                            comm_ptr->node_comm);
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
        MPIR_COLL_INNER(Allreduce, coll_sig, MPI_IN_PLACE, recvbuf, count, datatype, op,
                        comm_ptr->node_roots_comm);
    }

    /* now broadcast the result among local processes */
    if (comm_ptr->node_comm != NULL) {
        MPIR_COLL_INNER(Bcast, coll_sig, recvbuf, count, datatype, 0, comm_ptr->node_comm);
    }
    goto fn_exit;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
