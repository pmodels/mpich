/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/*
 * Broadcast-based neighbor allgather for cartesian communicators
 *
 */

int MPIR_Ineighbor_allgather_sched_cart_bcast(const void *sendbuf, int sendcount,
                                              MPI_Datatype sendtype, void *recvbuf,
                                              int recvcount, MPI_Datatype recvtype,
                                              MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int indegree, outdegree, weighted;
    int k, l;
    int *srcs, *dsts;
    MPI_Aint recvtype_extent;
    MPIR_CHKLMEM_DECL(2);

    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);

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

    MPIR_Topology *topo_ptr = MPIR_Topology_get(comm_ptr);
    MPIR_Assert(topo_ptr->kind == MPI_CART);

    for (k = 0; k < MPIR_Comm_size(comm_ptr); k++) {
        if (topo_ptr->subcomms[k] && MPIR_Comm_rank(topo_ptr->subcomms[k]) == 0) {
            MPIR_Ibcast_sched((void *) sendbuf, sendcount, sendtype, 0, topo_ptr->subcomms[k], s);
        }
    }
    for (k = 0; k < indegree; k++) {
        if (srcs[k] != MPI_PROC_NULL) {
            char *rb = ((char *) recvbuf) + k * recvcount * recvtype_extent;
            MPIR_Ibcast_sched(rb, recvcount, recvtype, 0, topo_ptr->subcomms[srcs[k]], s);
        }
    }

    MPIR_SCHED_BARRIER(s);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
