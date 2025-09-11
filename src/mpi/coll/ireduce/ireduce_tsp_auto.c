/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "coll_csel.h"

/* Provides a "flat" reduce that doesn't know anything about
 * hierarchy.  It will choose between several different algorithms
 * based on the given parameters. */
/* Remove this function when gentran algos are in json file */
int MPIR_Ireduce_sched_intra_tsp_flat_auto(const void *sendbuf, void *recvbuf,
                                           MPI_Aint count, MPI_Datatype datatype, MPI_Op op,
                                           int root, MPIR_Comm * comm_ptr, MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    int tree_type = MPIR_TREE_TYPE_KNOMIAL_1;
    int radix = 2, block_size = 0, buffer_per_child = 0;

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);

    /* Note: Use gentran_tree for short msg and gentran_reduce_scatter_gather for long msg
     * when we have gentran_reduce_scatter_gather implemented. For now all msg size use
     * gentran_tree algo */
    /* gentran tree with knomial tree type, radix 2 and pipeline block size 0 */
    mpi_errno = MPIR_TSP_Ireduce_sched_intra_tree(sendbuf, recvbuf, count,
                                                  datatype, op, root, comm_ptr,
                                                  tree_type, radix, block_size,
                                                  buffer_per_child, sched);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* sched version of CVAR and json based collective selection. Meant only for gentran scheduler */
int MPIR_TSP_Ireduce_sched_intra_tsp_auto(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                          MPI_Datatype datatype, MPI_Op op, int root,
                                          MPIR_Comm * comm_ptr, MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_TSP_Ireduce_sched_intra_tree(sendbuf, recvbuf, count, datatype, op, root,
                                          comm_ptr, MPIR_TREE_TYPE_KARY, 1,
                                          MPIR_CVAR_IREDUCE_RING_CHUNK_SIZE,
                                          MPIR_CVAR_IREDUCE_TREE_BUFFER_PER_CHILD, sched);


    return mpi_errno;
}
