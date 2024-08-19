/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"


int MPIR_Iallreduce_intra_sched_smp(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                    MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                    int coll_group, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int is_commutative;

    MPIR_Assert(MPIR_Comm_is_parent_comm(comm_ptr, coll_group));
    int local_rank = comm_ptr->subgroups[MPIR_SUBGROUP_NODE].rank;
    int local_size = comm_ptr->subgroups[MPIR_SUBGROUP_NODE].size;

    is_commutative = MPIR_Op_is_commutative(op);

    /* is the op commutative? We do SMP optimizations only if it is. */
    if (!is_commutative) {
        /* use flat fallback */
        mpi_errno =
            MPIR_Iallreduce_intra_sched_auto(sendbuf, recvbuf, count, datatype, op, comm_ptr,
                                             coll_group, s);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    /* on each node, do a reduce to the local root */
    if (local_size > 1) {
        /* take care of the MPI_IN_PLACE case. For reduce,
         * MPI_IN_PLACE is specified only on the root;
         * for allreduce it is specified on all processes. */
        if ((sendbuf == MPI_IN_PLACE) && (local_rank != 0)) {
            /* IN_PLACE and not root of reduce. Data supplied to this
             * allreduce is in recvbuf. Pass that as the sendbuf to reduce. */
            mpi_errno = MPIR_Ireduce_intra_sched_auto(recvbuf, NULL, count, datatype, op, 0,
                                                      comm_ptr, MPIR_SUBGROUP_NODE, s);
            MPIR_ERR_CHECK(mpi_errno);
        } else {
            mpi_errno = MPIR_Ireduce_intra_sched_auto(sendbuf, recvbuf, count, datatype, op, 0,
                                                      comm_ptr, MPIR_SUBGROUP_NODE, s);
            MPIR_ERR_CHECK(mpi_errno);
        }
        MPIR_SCHED_BARRIER(s);
    } else {
        /* only one process on the node. copy sendbuf to recvbuf */
        if (sendbuf != MPI_IN_PLACE) {
            mpi_errno = MPIR_Sched_copy(sendbuf, count, datatype, recvbuf, count, datatype, s);
            MPIR_ERR_CHECK(mpi_errno);
        }
        MPIR_SCHED_BARRIER(s);
    }

    /* now do an IN_PLACE allreduce among the local roots of all nodes */
    if (local_rank == 0) {
        mpi_errno = MPIR_Iallreduce_intra_sched_auto(MPI_IN_PLACE, recvbuf, count, datatype, op,
                                                     comm_ptr, MPIR_SUBGROUP_NODE_CROSS, s);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

    /* now broadcast the result among local processes */
    if (local_size > 1) {
        mpi_errno = MPIR_Ibcast_intra_sched_auto(recvbuf, count, datatype, 0,
                                                 comm_ptr, MPIR_SUBGROUP_NODE, s);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
