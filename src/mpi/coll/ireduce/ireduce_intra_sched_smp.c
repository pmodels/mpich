/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

int MPIR_Ireduce_intra_sched_smp(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                 MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm * comm_ptr,
                                 int coll_group, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int is_commutative;
    MPI_Aint true_lb, true_extent, extent;
    void *tmp_buf = NULL;

    MPIR_Assert(MPIR_Comm_is_parent_comm(comm_ptr, coll_group));
    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);
    int local_rank = comm_ptr->subgroups[MPIR_SUBGROUP_NODE].rank;
    int local_size = comm_ptr->subgroups[MPIR_SUBGROUP_NODE].size;
    int local_root = MPIR_Get_intranode_rank(comm_ptr, root);

    /* is the op commutative? We do SMP optimizations only if it is. */
    is_commutative = MPIR_Op_is_commutative(op);
    if (!is_commutative) {
        mpi_errno =
            MPIR_Ireduce_intra_sched_auto(sendbuf, recvbuf, count, datatype, op, root, comm_ptr,
                                          coll_group, s);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    /* Create a temporary buffer on local roots of all nodes */
    if (local_rank == 0) {
        MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
        MPIR_Datatype_get_extent_macro(datatype, extent);

        tmp_buf = MPIR_Sched_alloc_state(s, count * (MPL_MAX(extent, true_extent)));
        MPIR_ERR_CHKANDJUMP(!tmp_buf, mpi_errno, MPI_ERR_OTHER, "**nomem");
        /* adjust for potential negative lower bound in datatype */
        tmp_buf = (void *) ((char *) tmp_buf - true_lb);
    }

    /* do the intranode reduce on all nodes other than the root's node */
    if (local_size > 1 && local_root == -1) {
        mpi_errno = MPIR_Ireduce_intra_sched_auto(sendbuf, tmp_buf, count, datatype, op, 0,
                                                  comm_ptr, MPIR_SUBGROUP_NODE, s);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

    /* do the internode reduce to the root's node */
    if (local_rank == 0) {
        int inter_root = MPIR_Get_internode_rank(comm_ptr, root);
        if (local_root < 0) {
            /* I am not on root's node.  Use tmp_buf if we
             * participated in the first reduce, otherwise use sendbuf */
            const void *buf = (local_size > 1 ? tmp_buf : sendbuf);
            mpi_errno = MPIR_Ireduce_intra_sched_auto(buf, NULL, count, datatype, op, inter_root,
                                                      comm_ptr, MPIR_SUBGROUP_NODE_CROSS, s);
            MPIR_ERR_CHECK(mpi_errno);
            MPIR_SCHED_BARRIER(s);
        } else {        /* I am on root's node. I have not participated in the earlier reduce. */
            if (local_rank != local_root) {
                /* I am not the root though. I don't have a valid recvbuf.
                 * Use tmp_buf as recvbuf. */

                mpi_errno = MPIR_Ireduce_intra_sched_auto(sendbuf, tmp_buf, count, datatype,
                                                          op, inter_root, comm_ptr,
                                                          MPIR_SUBGROUP_NODE_CROSS, s);
                MPIR_ERR_CHECK(mpi_errno);
                MPIR_SCHED_BARRIER(s);

                /* point sendbuf at tmp_buf to make final intranode reduce easy */
                sendbuf = tmp_buf;
            } else {
                /* I am the root. in_place is automatically handled. */

                mpi_errno = MPIR_Ireduce_intra_sched_auto(sendbuf, recvbuf, count, datatype,
                                                          op, inter_root, comm_ptr,
                                                          MPIR_SUBGROUP_NODE_CROSS, s);
                MPIR_ERR_CHECK(mpi_errno);
                MPIR_SCHED_BARRIER(s);

                /* set sendbuf to MPI_IN_PLACE to make final intranode reduce easy. */
                sendbuf = MPI_IN_PLACE;
            }
        }
    }

    /* do the intranode reduce on the root's node */
    if (local_size > 1 && MPIR_Get_intranode_rank(comm_ptr, root) != -1) {
        mpi_errno = MPIR_Ireduce_intra_sched_auto(sendbuf, recvbuf, count, datatype,
                                                  op, local_root, comm_ptr, MPIR_SUBGROUP_NODE, s);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }


  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
