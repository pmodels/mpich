/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"


int MPIR_Iscan_intra_sched_smp(const void *sendbuf, void *recvbuf, MPI_Aint count,
                               MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                               int coll_group, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint true_extent, true_lb, extent;
    void *tempbuf = NULL;
    void *prefulldata = NULL;
    void *localfulldata = NULL;

    MPIR_Assert(MPIR_Comm_is_parent_comm(comm_ptr, coll_group));
    int local_rank = comm_ptr->subgroups[MPIR_SUBGROUP_NODE].rank;
    int local_size = comm_ptr->subgroups[MPIR_SUBGROUP_NODE].size;
    int inter_rank = MPIR_Get_internode_rank(comm_ptr, comm_ptr->rank);

    /* In order to use the SMP-aware algorithm, the "op" can be
     * either commutative or non-commutative, but we require a
     * communicator in which all the nodes contain processes with
     * consecutive ranks. */

    if (!MPII_Comm_is_node_consecutive(comm_ptr)) {
        /* We can't use the SMP-aware algorithm, use the non-SMP-aware
         * one */
        return MPIR_Iscan_intra_sched_recursive_doubling(sendbuf, recvbuf, count, datatype, op,
                                                         comm_ptr, coll_group, s);
    }

    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
    MPIR_Datatype_get_extent_macro(datatype, extent);

    tempbuf = MPIR_Sched_alloc_state(s, count * (MPL_MAX(extent, true_extent)));
    MPIR_ERR_CHKANDJUMP(!tempbuf, mpi_errno, MPI_ERR_OTHER, "**nomem");
    tempbuf = (void *) ((char *) tempbuf - true_lb);

    /* Create prefulldata and localfulldata on local roots of all nodes */
    if (local_rank == 0) {
        prefulldata = MPIR_Sched_alloc_state(s, count * (MPL_MAX(extent, true_extent)));
        MPIR_ERR_CHKANDJUMP(!prefulldata, mpi_errno, MPI_ERR_OTHER, "**nomem");
        prefulldata = (void *) ((char *) prefulldata - true_lb);

        if (local_size > 1) {
            localfulldata = MPIR_Sched_alloc_state(s, count * (MPL_MAX(extent, true_extent)));
            MPIR_ERR_CHKANDJUMP(!localfulldata, mpi_errno, MPI_ERR_OTHER, "**nomem");
            localfulldata = (void *) ((char *) localfulldata - true_lb);
        }
    }

    /* perform intranode scan to get temporary result in recvbuf. if there is only
     * one process, just copy the raw data. */
    if (local_size > 1) {
        mpi_errno = MPIR_Iscan_intra_sched_auto(sendbuf, recvbuf, count, datatype, op,
                                                comm_ptr, MPIR_SUBGROUP_NODE, s);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    } else if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Sched_copy(sendbuf, count, datatype, recvbuf, count, datatype, s);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

    /* get result from local node's last processor which
     * contains the reduce result of the whole node. Name it as
     * localfulldata. For example, localfulldata from node 1 contains
     * reduced data of rank 1,2,3. */
    if (local_rank == 0 && local_size > 1) {
        mpi_errno = MPIR_Sched_recv(localfulldata, count, datatype, (local_size - 1),
                                    comm_ptr, MPIR_SUBGROUP_NODE, s);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    } else if (local_rank != 0 && local_size > 1 && local_rank == local_size - 1) {
        mpi_errno = MPIR_Sched_send(recvbuf, count, datatype, 0, comm_ptr, MPIR_SUBGROUP_NODE, s);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    } else if (local_rank == 0) {
        localfulldata = recvbuf;
    }

    /* do scan on localfulldata to prefulldata. for example,
     * prefulldata on rank 4 contains reduce result of ranks
     * 1,2,3,4,5,6. it will be sent to rank 7 which is the
     * main process of node 3. */
    if (local_rank == 0) {
        int inter_size = comm_ptr->subgroups[MPIR_SUBGROUP_NODE_CROSS].size;

        mpi_errno = MPIR_Iscan_intra_sched_auto(localfulldata, prefulldata, count, datatype, op,
                                                comm_ptr, MPIR_SUBGROUP_NODE_CROSS, s);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_SCHED_BARRIER(s);

        if (inter_rank != inter_size - 1) {
            mpi_errno = MPIR_Sched_send(prefulldata, count, datatype, (inter_rank + 1),
                                        comm_ptr, MPIR_SUBGROUP_NODE_CROSS, s);
            MPIR_ERR_CHECK(mpi_errno);
            MPIR_SCHED_BARRIER(s);
        }
        if (inter_rank != 0) {
            mpi_errno = MPIR_Sched_recv(tempbuf, count, datatype, (inter_rank - 1),
                                        comm_ptr, MPIR_SUBGROUP_NODE_CROSS, s);
            MPIR_ERR_CHECK(mpi_errno);
            MPIR_SCHED_BARRIER(s);
        }
    }

    /* now tempbuf contains all the data needed to get the correct
     * scan result. for example, to node 3, it will have reduce result
     * of rank 1,2,3,4,5,6 in tempbuf.
     * then we should broadcast this result in the local node, and
     * reduce it with recvbuf to get final result if necessary. */

    if (inter_rank != 0) {
        /* we aren't on "node 0", so our node leader (possibly us) received
         * "prefulldata" from another leader into "tempbuf" */

        if (local_size > 1) {
            mpi_errno = MPIR_Ibcast_intra_sched_auto(tempbuf, count, datatype, 0,
                                                     comm_ptr, MPIR_SUBGROUP_NODE, s);
            MPIR_ERR_CHECK(mpi_errno);
            MPIR_SCHED_BARRIER(s);
        }

        /* do reduce on tempbuf and recvbuf, finish scan. */
        mpi_errno = MPIR_Sched_reduce(tempbuf, recvbuf, count, datatype, op, s);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
