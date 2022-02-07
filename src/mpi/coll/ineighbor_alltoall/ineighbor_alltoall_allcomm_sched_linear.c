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
    int k, l;
    int *srcs, *dsts;
    MPI_Aint sendtype_extent, recvtype_extent;
    MPIR_CHKLMEM_DECL(2);

    MPIR_Datatype_get_extent_macro(sendtype, sendtype_extent);
    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);

    mpi_errno = MPIR_Topo_canon_nhb_count(comm_ptr, &indegree, &outdegree, &weighted);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_CHKLMEM_MALLOC(srcs, int *, indegree * sizeof(int), mpi_errno, "srcs", MPL_MEM_COMM);
    MPIR_CHKLMEM_MALLOC(dsts, int *, outdegree * sizeof(int), mpi_errno, "dsts", MPL_MEM_COMM);
    mpi_errno = MPIR_Topo_canon_nhb(comm_ptr,
                                    indegree, srcs, MPI_UNWEIGHTED,
                                    outdegree, dsts, MPI_UNWEIGHTED);
    MPIR_ERR_CHECK(mpi_errno);

    for (k = 0; k < outdegree; ++k) {
        char *sb = ((char *) sendbuf) + k * sendcount * sendtype_extent;
        mpi_errno = MPIR_Sched_send(sb, sendcount, sendtype, dsts[k], comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* Cartesian graph may result in multiple edges going to the same process when it is
     * periodic and dim is 1 or 2, in which case, both left and right neighbor (in a circular sense) points to
     * self or the other process. When that occurs, we are sending two messages to the
     * same receiver and receiving two messages from the same sender. Both messages are using
     * the same tag, thus we need reverse the order of recvs to ensure correct matching.
     * In the example of 1-dim cartesian graph, if we send in the order of left and right,
     * the target need receive in the order of right and left to receive the correct messages.
     *
     * Note: duplicate graph edges can only result from MPI_Cart_create when certain
     * dimension is periodic and size is 1 or 2. And the resulting graph edges will be
     * in fixed orders, which the code here takes advantage of. A general graph will not
     * contain duplicated edges, and the order of send and recv should not matter.
     */
    for (l = indegree - 1; l >= 0; l--) {
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
