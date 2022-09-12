/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* Provides a "flat" reduce that doesn't know anything about
 * hierarchy.  It will choose between several different algorithms
 * based on the given parameters. */
/* Remove this function when gentran algos are in json file */
static int MPIR_Ireduce_sched_intra_tsp_flat_auto(const void *sendbuf, void *recvbuf,
                                                  MPI_Aint count, MPI_Datatype datatype, MPI_Op op,
                                                  int root, MPIR_Comm * comm_ptr,
                                                  MPIR_TSP_sched_t sched)
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

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__IREDUCE,
        .comm_ptr = comm_ptr,

        .u.ireduce.sendbuf = sendbuf,
        .u.ireduce.recvbuf = recvbuf,
        .u.ireduce.count = count,
        .u.ireduce.datatype = datatype,
        .u.ireduce.op = op,
        .u.ireduce.root = root,
    };
    MPII_Csel_container_s *cnt;

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);

    switch (MPIR_CVAR_IREDUCE_INTRA_ALGORITHM) {
        case MPIR_CVAR_IREDUCE_INTRA_ALGORITHM_tsp_tree:
            /*Only knomial_1 tree supports non-commutative operations */
            MPII_COLLECTIVE_FALLBACK_CHECK(comm_ptr->rank, MPIR_Op_is_commutative(op) ||
                                           MPIR_Ireduce_tree_type == MPIR_TREE_TYPE_KNOMIAL_1,
                                           mpi_errno, "Ireduce gentran_tree cannot be applied.\n");
            mpi_errno =
                MPIR_TSP_Ireduce_sched_intra_tree(sendbuf, recvbuf, count, datatype, op, root,
                                                  comm_ptr, MPIR_Ireduce_tree_type,
                                                  MPIR_CVAR_IREDUCE_TREE_KVAL,
                                                  MPIR_CVAR_IREDUCE_TREE_PIPELINE_CHUNK_SIZE,
                                                  MPIR_CVAR_IREDUCE_TREE_BUFFER_PER_CHILD, sched);
            break;

        case MPIR_CVAR_IREDUCE_INTRA_ALGORITHM_tsp_ring:
            mpi_errno =
                MPIR_TSP_Ireduce_sched_intra_tree(sendbuf, recvbuf, count, datatype, op, root,
                                                  comm_ptr, MPIR_TREE_TYPE_KARY, 1,
                                                  MPIR_CVAR_IREDUCE_RING_CHUNK_SIZE,
                                                  MPIR_CVAR_IREDUCE_TREE_BUFFER_PER_CHILD, sched);
            break;

        default:
            cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
            MPIR_Assert(cnt);

            switch (cnt->id) {
                case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_intra_tsp_tree:
                    mpi_errno =
                        MPIR_TSP_Ireduce_sched_intra_tree(sendbuf, recvbuf, count, datatype, op,
                                                          root, comm_ptr,
                                                          cnt->u.ireduce.intra_tsp_tree.tree_type,
                                                          cnt->u.ireduce.intra_tsp_tree.k,
                                                          cnt->u.ireduce.intra_tsp_tree.chunk_size,
                                                          cnt->u.ireduce.
                                                          intra_tsp_tree.buffer_per_child, sched);
                    break;

                case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_intra_tsp_ring:
                    mpi_errno =
                        MPIR_TSP_Ireduce_sched_intra_tree(sendbuf, recvbuf, count, datatype, op,
                                                          root, comm_ptr, MPIR_TREE_TYPE_KARY, 1,
                                                          cnt->u.ireduce.intra_tsp_ring.chunk_size,
                                                          cnt->u.ireduce.
                                                          intra_tsp_ring.buffer_per_child, sched);
                    break;

                default:
                    /* Replace this call with MPIR_Assert(0) when json files have gentran algos */
                    mpi_errno =
                        MPIR_Ireduce_sched_intra_tsp_flat_auto(sendbuf, recvbuf, count,
                                                               datatype, op, root, comm_ptr, sched);
                    break;
            }
    }
    MPIR_ERR_CHECK(mpi_errno);
    goto fn_exit;

  fallback:
    mpi_errno =
        MPIR_TSP_Ireduce_sched_intra_tree(sendbuf, recvbuf, count, datatype, op, root,
                                          comm_ptr, MPIR_TREE_TYPE_KARY, 1,
                                          MPIR_CVAR_IREDUCE_RING_CHUNK_SIZE,
                                          MPIR_CVAR_IREDUCE_TREE_BUFFER_PER_CHILD, sched);


  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
