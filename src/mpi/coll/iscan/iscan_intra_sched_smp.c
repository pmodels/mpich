/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"


int MPIR_Iscan_intra_sched_smp(const void *sendbuf, void *recvbuf, MPI_Aint count,
                               MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                               MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int rank = comm_ptr->rank;
    MPIR_Comm *node_comm;
    MPIR_Comm *roots_comm;
    MPI_Aint true_extent, true_lb, extent;
    void *tempbuf = NULL;
    void *prefulldata = NULL;
    void *localfulldata = NULL;

    /* In order to use the SMP-aware algorithm, the "op" can be
     * either commutative or non-commutative, but we require a
     * communicator in which all the nodes contain processes with
     * consecutive ranks. */

    if (!MPII_Comm_is_node_consecutive(comm_ptr)) {
        /* We can't use the SMP-aware algorithm, use the non-SMP-aware
         * one */
        return MPIR_Iscan_intra_sched_recursive_doubling(sendbuf, recvbuf, count, datatype, op,
                                                         comm_ptr, s);
    }

    node_comm = comm_ptr->node_comm;
    roots_comm = comm_ptr->node_roots_comm;

    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
    MPIR_Datatype_get_extent_macro(datatype, extent);

    tempbuf = MPIR_Sched_alloc_state(s, count * (MPL_MAX(extent, true_extent)));
    MPIR_ERR_CHKANDJUMP(!tempbuf, mpi_errno, MPI_ERR_OTHER, "**nomem");
    tempbuf = (void *) ((char *) tempbuf - true_lb);

    /* Create prefulldata and localfulldata on local roots of all nodes */
    if (comm_ptr->node_roots_comm != NULL) {
        prefulldata = MPIR_Sched_alloc_state(s, count * (MPL_MAX(extent, true_extent)));
        MPIR_ERR_CHKANDJUMP(!prefulldata, mpi_errno, MPI_ERR_OTHER, "**nomem");
        prefulldata = (void *) ((char *) prefulldata - true_lb);

        if (node_comm != NULL) {
            localfulldata = MPIR_Sched_alloc_state(s, count * (MPL_MAX(extent, true_extent)));
            MPIR_ERR_CHKANDJUMP(!localfulldata, mpi_errno, MPI_ERR_OTHER, "**nomem");
            localfulldata = (void *) ((char *) localfulldata - true_lb);
        }
    }

    /* perform intranode scan to get temporary result in recvbuf. if there is only
     * one process, just copy the raw data. */
    if (node_comm != NULL) {
        mpi_errno =
            MPIR_Iscan_intra_sched_auto(sendbuf, recvbuf, count, datatype, op, node_comm, s);
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
    if (roots_comm != NULL && node_comm != NULL) {
        mpi_errno = MPIR_Sched_recv(localfulldata, count, datatype,
                                    (node_comm->local_size - 1), node_comm, s);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    } else if (roots_comm == NULL && node_comm != NULL &&
               node_comm->rank == node_comm->local_size - 1) {
        mpi_errno = MPIR_Sched_send(recvbuf, count, datatype, 0, node_comm, s);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    } else if (roots_comm != NULL) {
        localfulldata = recvbuf;
    }

    /* do scan on localfulldata to prefulldata. for example,
     * prefulldata on rank 4 contains reduce result of ranks
     * 1,2,3,4,5,6. it will be sent to rank 7 which is the
     * main process of node 3. */
    if (roots_comm != NULL) {
        /* FIXME just use roots_comm->rank instead */
        int roots_rank = MPIR_Get_internode_rank(comm_ptr, rank);
        MPIR_Assert(roots_rank == roots_comm->rank);

        mpi_errno =
            MPIR_Iscan_intra_sched_auto(localfulldata, prefulldata, count, datatype, op, roots_comm,
                                        s);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_SCHED_BARRIER(s);

        if (roots_rank != roots_comm->local_size - 1) {
            mpi_errno =
                MPIR_Sched_send(prefulldata, count, datatype, (roots_rank + 1), roots_comm, s);
            MPIR_ERR_CHECK(mpi_errno);
            MPIR_SCHED_BARRIER(s);
        }
        if (roots_rank != 0) {
            mpi_errno = MPIR_Sched_recv(tempbuf, count, datatype, (roots_rank - 1), roots_comm, s);
            MPIR_ERR_CHECK(mpi_errno);
            MPIR_SCHED_BARRIER(s);
        }
    }

    /* now tempbuf contains all the data needed to get the correct
     * scan result. for example, to node 3, it will have reduce result
     * of rank 1,2,3,4,5,6 in tempbuf.
     * then we should broadcast this result in the local node, and
     * reduce it with recvbuf to get final result if necessary. */

    if (MPIR_Get_internode_rank(comm_ptr, rank) != 0) {
        /* we aren't on "node 0", so our node leader (possibly us) received
         * "prefulldata" from another leader into "tempbuf" */

        if (node_comm != NULL) {
            mpi_errno = MPIR_Ibcast_intra_sched_auto(tempbuf, count, datatype, 0, node_comm, s);
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
