/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"


#undef FUNCNAME
#define FUNCNAME MPIR_Iallreduce_sched_intra_smp
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallreduce_sched_intra_smp(const void *sendbuf, void *recvbuf, int count,
                                    MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                    MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int is_commutative;
    MPIR_Comm *nc;
    MPIR_Comm *nrc;

    MPIR_Assert(MPIR_Comm_is_node_aware(comm_ptr));

    nc = comm_ptr->node_comm;
    nrc = comm_ptr->node_roots_comm;

    is_commutative = MPIR_Op_is_commutative(op);

    /* is the op commutative? We do SMP optimizations only if it is. */
    if (!is_commutative) {
        /* use flat fallback */
        mpi_errno =
            MPIR_Iallreduce_sched_intra_auto(sendbuf, recvbuf, count, datatype, op, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        goto fn_exit;
    }

    /* on each node, do a reduce to the local root */
    if (nc != NULL) {
        /* take care of the MPI_IN_PLACE case. For reduce,
         * MPI_IN_PLACE is specified only on the root;
         * for allreduce it is specified on all processes. */
        if ((sendbuf == MPI_IN_PLACE) && (comm_ptr->node_comm->rank != 0)) {
            /* IN_PLACE and not root of reduce. Data supplied to this
             * allreduce is in recvbuf. Pass that as the sendbuf to reduce. */
            mpi_errno = MPIR_Ireduce_sched(recvbuf, NULL, count, datatype, op, 0, nc, s);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        } else {
            mpi_errno = MPIR_Ireduce_sched(sendbuf, recvbuf, count, datatype, op, 0, nc, s);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }
        MPIR_SCHED_BARRIER(s);
    } else {
        /* only one process on the node. copy sendbuf to recvbuf */
        if (sendbuf != MPI_IN_PLACE) {
            mpi_errno = MPIR_Sched_copy(sendbuf, count, datatype, recvbuf, count, datatype, s);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }
        MPIR_SCHED_BARRIER(s);
    }

    /* now do an IN_PLACE allreduce among the local roots of all nodes */
    if (nrc != NULL) {
        mpi_errno = MPIR_Iallreduce_sched(MPI_IN_PLACE, recvbuf, count, datatype, op, nrc, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

    /* now broadcast the result among local processes */
    if (comm_ptr->node_comm != NULL) {
        mpi_errno = MPIR_Ibcast_sched(recvbuf, count, datatype, 0, nc, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
