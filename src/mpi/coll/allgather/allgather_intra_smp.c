/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* This is a special "allgather" algorithm that relies on node_comm and node_roots_comm
 * to allgather results. The results may NOT be in the order of ranks but reordered by
 * "nodes". This is useful when the comm is not setup for all-to-all connections or we
 * want route network traffic through node_roots, and when the ordering of results is not
 * critical. For example, the id maybe embedded in the data. Or the comm may already in
 * "node-order".
 *
 * This algorithm is used in ch4 MPIDI_NM_comm_addr_exchange.
 */
int MPIR_Allgather_intra_smp_no_order(const void *sendbuf, MPI_Aint sendcount,
                                      MPI_Datatype sendtype,
                                      void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                      MPIR_Comm * comm_ptr, MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_CHKLMEM_DECL();

    int local_size = comm_ptr->num_local;
    int local_rank = comm_ptr->local_rank;
    int external_size = comm_ptr->num_external;
    int external_rank = comm_ptr->external_rank;

    MPIR_Comm *node_comm, *node_roots_comm;
    node_comm = MPIR_Comm_get_node_comm(comm_ptr);
    MPIR_Assert(node_comm);
    if (local_rank == 0) {
        node_roots_comm = MPIR_Comm_get_node_roots_comm(comm_ptr);
        MPIR_Assert(node_roots_comm);
    }

    MPI_Aint total_count = comm_ptr->local_size * recvcount;

    /* -- roots survey the node sizes -- */
    /* TODO: alternatively compute from node map and skip 1 allgather */
    MPI_Aint *counts, *displs;
    if (local_rank == 0) {
        MPIR_CHKLMEM_MALLOC(counts, external_size * sizeof(MPI_Aint));
        MPIR_CHKLMEM_MALLOC(displs, external_size * sizeof(MPI_Aint));
        MPI_Aint my_count = local_size;
        mpi_errno = MPIR_Allgather_impl(&my_count, 1, MPIR_AINT_INTERNAL,
                                        counts, 1, MPIR_AINT_INTERNAL,
                                        node_roots_comm, MPIR_ERR_NONE);
        MPIR_ERR_CHECK(mpi_errno);

        MPI_Aint cur_disp = 0;
        for (int i = 0; i < external_size; i++) {
            counts[i] *= recvcount;
            displs[i] = cur_disp;
            cur_disp += counts[i];
        }
        MPIR_Assert(cur_disp == total_count);
    }

    /* -- gather over node -- */
    void *local_recvbuf = NULL;
    if (local_rank == 0) {
        MPI_Aint recvtype_extent;
        MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);
        local_recvbuf = (char *) recvbuf + displs[external_rank] * recvtype_extent;
    }
    mpi_errno = MPIR_Gather_impl(sendbuf, sendcount, sendtype,
                                 local_recvbuf, recvcount, recvtype, 0, node_comm, MPIR_ERR_NONE);
    MPIR_ERR_CHECK(mpi_errno);

    /* -- allgatherv over node roots -- */
    if (local_rank == 0) {
        mpi_errno = MPIR_Allgatherv_impl(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
                                         recvbuf, counts, displs, recvtype,
                                         node_roots_comm, MPIR_ERR_NONE);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* -- bcast over node -- */
    mpi_errno = MPIR_Bcast_impl(recvbuf, total_count, recvtype, 0, node_comm, MPIR_ERR_NONE);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
