/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/*
 * Linear
 *
 * Simple send to each outgoing neighbor and recv from each incoming
 * neighbor.
 */

int MPIR_Ineighbor_alltoallw_sched_allcomm_linear(const void *sendbuf, const int sendcounts[],
                                                  const MPI_Aint sdispls[],
                                                  const MPI_Datatype sendtypes[], void *recvbuf,
                                                  const int recvcounts[], const MPI_Aint rdispls[],
                                                  const MPI_Datatype recvtypes[],
                                                  MPIR_Comm * comm_ptr, MPIR_Sched_element_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int indegree, outdegree, weighted;
    int k, l;
    int *srcs, *dsts;
    MPIR_CHKLMEM_DECL(2);

    mpi_errno = MPIR_Topo_canon_nhb_count(comm_ptr, &indegree, &outdegree, &weighted);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    MPIR_CHKLMEM_MALLOC(srcs, int *, indegree * sizeof(int), mpi_errno, "srcs", MPL_MEM_COMM);
    MPIR_CHKLMEM_MALLOC(dsts, int *, outdegree * sizeof(int), mpi_errno, "dsts", MPL_MEM_COMM);
    mpi_errno = MPIR_Topo_canon_nhb(comm_ptr,
                                    indegree, srcs, MPI_UNWEIGHTED,
                                    outdegree, dsts, MPI_UNWEIGHTED);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    for (k = 0; k < outdegree; ++k) {
        char *sb;

        sb = ((char *) sendbuf) + sdispls[k];
        mpi_errno = MPIR_Sched_element_send(sb, sendcounts[k], sendtypes[k], dsts[k], comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    for (l = 0; l < indegree; ++l) {
        char *rb;

        rb = ((char *) recvbuf) + rdispls[l];
        mpi_errno = MPIR_Sched_element_recv(rb, recvcounts[l], recvtypes[l], srcs[l], comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    MPIR_SCHED_BARRIER(s);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
