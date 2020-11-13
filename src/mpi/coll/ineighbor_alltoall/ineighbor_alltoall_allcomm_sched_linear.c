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

int MPIR_Ineighbor_alltoall_allcomm_sched_linear(const void *sendbuf, int sendcount,
                                                 MPI_Datatype sendtype, void *recvbuf,
                                                 int recvcount, MPI_Datatype recvtype,
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

    /* receive needs to happen in the opposite order of sends.  This
     * is to cover the case of Cartesian graphs where the same process
     * might be both the left and right neighbor.  In this case, we
     * need to make sure the first message (which is from the right
     * neighbor) is received in the last buffer. */
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
