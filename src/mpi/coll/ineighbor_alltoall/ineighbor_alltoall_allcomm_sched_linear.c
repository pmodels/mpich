/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
 * Linear
 *
 * Simple send to each outgoing neighbor and recv from each incoming
 * neighbor.
 */

int MPIR_Ineighbor_alltoall_allcomm_sched_linear(const void *sendbuf, MPI_Aint sendcount,
                                                 MPI_Datatype sendtype, void *recvbuf,
                                                 MPI_Aint recvcount, MPI_Datatype recvtype,
                                                 MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int indegree, outdegree, weighted;
    int i, k, l;
    int swap_recv_pairs;
    int *srcs, *dsts;
    MPI_Aint sendtype_extent, recvtype_extent;
    MPIR_CHKLMEM_DECL();

    MPIR_Datatype_get_extent_macro(sendtype, sendtype_extent);
    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);

    mpi_errno = MPIR_Topo_canon_nhb_count(comm_ptr, &indegree, &outdegree, &weighted);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_CHKLMEM_MALLOC(srcs, indegree * sizeof(int));
    MPIR_CHKLMEM_MALLOC(dsts, outdegree * sizeof(int));
    mpi_errno = MPIR_Topo_canon_nhb(comm_ptr,
                                    indegree, srcs, MPI_UNWEIGHTED,
                                    outdegree, dsts, MPI_UNWEIGHTED);
    MPIR_ERR_CHECK(mpi_errno);

    for (k = 0; k < outdegree; ++k) {
        char *sb = ((char *) sendbuf) + k * sendcount * sendtype_extent;
        mpi_errno = MPIR_Sched_send(sb, sendcount, sendtype, dsts[k], comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* All the messages of the collective share one tag, so messages exchanged
     * with the same MPI process are matched in the order they are posted, and
     * the neighbor lists may contain the same MPI process more than once: a
     * Cartesian dimension that is periodic and of size 1 or 2 has
     * rank_source == rank_dest, and a (distributed) graph topology may simply
     * repeat an edge.  On a Cartesian communicator MPI-4.1 Example 8.10
     * requires the block sent in the negative direction of dimension d to be
     * received into block 2*d+1 of the neighbor and vice versa, which is
     * obtained by posting the receives with the two blocks of each dimension
     * swapped; on the graph topologies the equivalent code of MPI-4.1
     * Section 8.6 matches the k-th edge to an MPI process with the k-th edge
     * from it, so the receives are posted in list order.  See
     * MPIR_Topo_nhb_swap_recv_pairs() in src/mpi/topo/topoutil.c. */
    swap_recv_pairs = MPIR_Topo_nhb_swap_recv_pairs(comm_ptr);
    MPIR_Assert(!swap_recv_pairs || indegree % 2 == 0);
    for (i = 0; i < indegree; ++i) {
        l = swap_recv_pairs ? (i ^ 1) : i;
        char *rb = ((char *) recvbuf) + l * recvcount * recvtype_extent;
        mpi_errno = MPIR_Sched_recv(rb, recvcount, recvtype, srcs[l], comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
    }

    MPIR_SCHED_BARRIER(s);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
