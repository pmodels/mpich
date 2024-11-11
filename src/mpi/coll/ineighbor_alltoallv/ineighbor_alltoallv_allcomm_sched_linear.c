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

int MPIR_Ineighbor_alltoallv_allcomm_sched_linear(const void *sendbuf, const MPI_Aint sendcounts[],
                                                  const MPI_Aint sdispls[], MPI_Datatype sendtype,
                                                  void *recvbuf, const MPI_Aint recvcounts[],
                                                  const MPI_Aint rdispls[], MPI_Datatype recvtype,
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
        char *sb = ((char *) sendbuf) + sdispls[k] * sendtype_extent;
        mpi_errno =
            MPIR_Sched_send(sb, sendcounts[k], sendtype, dsts[k], comm_ptr, MPIR_SUBGROUP_NONE, s);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* need reverse the order to ensure matching when the graph is from MPI_Cart_create and
     * the n-th dimension is periodic and the size is 1 or 2.
     * ref. ineighbor_alltoall_allcomm_sched_linear.c */
    for (l = indegree - 1; l >= 0; l--) {
        char *rb = ((char *) recvbuf) + rdispls[l] * recvtype_extent;
        mpi_errno =
            MPIR_Sched_recv(rb, recvcounts[l], recvtype, srcs[l], comm_ptr, MPIR_SUBGROUP_NONE, s);
        MPIR_ERR_CHECK(mpi_errno);
    }

    MPIR_SCHED_BARRIER(s);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
