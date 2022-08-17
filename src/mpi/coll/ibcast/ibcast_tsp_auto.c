/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "algo_common.h"
#include "treealgo.h"

/* Provides a "flat" broadcast that doesn't know anything about
 * hierarchy.  It will choose between several different algorithms
 * based on the given parameters. */
/* Remove this function when gentran algos are in json file */
static int MPIR_Ibcast_sched_intra_tsp_flat_auto(void *buffer, MPI_Aint count,
                                                 MPI_Datatype datatype, int root,
                                                 MPIR_Comm * comm_ptr, MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    int comm_size;
    MPI_Aint type_size, nbytes;
    int tree_type = MPIR_TREE_TYPE_KNOMIAL_1;
    int radix = 2, scatterv_k = 2, allgatherv_k = 2, block_size = 0;

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);

    comm_size = comm_ptr->local_size;
    MPIR_Datatype_get_size_macro(datatype, type_size);
    nbytes = type_size * count;

    /* simplistic implementation for now */
    if ((nbytes < MPIR_CVAR_BCAST_SHORT_MSG_SIZE) || (comm_size < MPIR_CVAR_BCAST_MIN_PROCS)) {
        /* gentran tree with knomial tree type, radix 2 and pipeline block size 0 */
        mpi_errno = MPIR_TSP_Ibcast_sched_intra_tree(buffer, count, datatype, root, comm_ptr,
                                                     tree_type, radix, block_size, sched);
    } else {
        /* gentran scatterv recexch allgather with radix 2 */
        mpi_errno =
            MPIR_TSP_Ibcast_sched_intra_scatterv_allgatherv(buffer, count, datatype, root,
                                                            comm_ptr,
                                                            MPIR_CVAR_IALLGATHERV_INTRA_ALGORITHM_tsp_recexch_doubling,
                                                            scatterv_k, allgatherv_k, sched);
    }
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* sched version of CVAR and json based collective selection. Meant only for gentran scheduler */
int MPIR_TSP_Ibcast_sched_intra_tsp_auto(void *buffer, MPI_Aint count, MPI_Datatype datatype,
                                         int root, MPIR_Comm * comm_ptr, MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__IBCAST,
        .comm_ptr = comm_ptr,

        .u.ibcast.buffer = buffer,
        .u.ibcast.count = count,
        .u.ibcast.datatype = datatype,
        .u.ibcast.root = root,
    };
    MPII_Csel_container_s *cnt;

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);

    switch (MPIR_CVAR_IBCAST_INTRA_ALGORITHM) {
        case MPIR_CVAR_IBCAST_INTRA_ALGORITHM_tsp_tree:
            mpi_errno =
                MPIR_TSP_Ibcast_sched_intra_tree(buffer, count, datatype, root, comm_ptr,
                                                 MPIR_Ibcast_tree_type,
                                                 MPIR_CVAR_IBCAST_TREE_KVAL,
                                                 MPIR_CVAR_IBCAST_TREE_PIPELINE_CHUNK_SIZE, sched);
            break;

        case MPIR_CVAR_IBCAST_INTRA_ALGORITHM_tsp_scatterv_recexch_allgatherv:
            mpi_errno =
                MPIR_TSP_Ibcast_sched_intra_scatterv_allgatherv(buffer, count, datatype,
                                                                root, comm_ptr,
                                                                MPIR_CVAR_IALLGATHERV_INTRA_ALGORITHM_tsp_recexch_doubling,
                                                                MPIR_CVAR_IBCAST_SCATTERV_KVAL,
                                                                MPIR_CVAR_IBCAST_ALLGATHERV_RECEXCH_KVAL,
                                                                sched);
            break;

        case MPIR_CVAR_IBCAST_INTRA_ALGORITHM_tsp_scatterv_ring_allgatherv:
            mpi_errno =
                MPIR_TSP_Ibcast_sched_intra_scatterv_ring_allgatherv(buffer, count, datatype,
                                                                     root, comm_ptr, 1, sched);
            break;

        case MPIR_CVAR_IBCAST_INTRA_ALGORITHM_tsp_ring:
            mpi_errno =
                MPIR_TSP_Ibcast_sched_intra_tree(buffer, count, datatype, root, comm_ptr,
                                                 MPIR_TREE_TYPE_KARY, 1,
                                                 MPIR_CVAR_IBCAST_RING_CHUNK_SIZE, sched);
            break;

        default:
            cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
            MPIR_Assert(cnt);

            switch (cnt->id) {
                case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_tsp_tree:
                    mpi_errno =
                        MPIR_TSP_Ibcast_sched_intra_tree(buffer, count, datatype, root, comm_ptr,
                                                         cnt->u.ibcast.intra_tsp_tree.tree_type,
                                                         cnt->u.ibcast.intra_tsp_tree.k,
                                                         cnt->u.ibcast.intra_tsp_tree.chunk_size,
                                                         sched);
                    break;
                case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_tsp_scatterv_recexch_allgatherv:
                    mpi_errno =
                        MPIR_TSP_Ibcast_sched_intra_scatterv_allgatherv(buffer, count, datatype,
                                                                        root, comm_ptr,
                                                                        MPIR_CVAR_IALLGATHERV_INTRA_ALGORITHM_tsp_recexch_doubling,
                                                                        cnt->u.
                                                                        ibcast.intra_tsp_scatterv_recexch_allgatherv.scatterv_k,
                                                                        cnt->u.
                                                                        ibcast.intra_tsp_scatterv_recexch_allgatherv.allgatherv_k,
                                                                        sched);
                    break;

                case MPIR_CVAR_IBCAST_INTRA_ALGORITHM_tsp_scatterv_ring_allgatherv:
                    mpi_errno =
                        MPIR_TSP_Ibcast_sched_intra_scatterv_ring_allgatherv(buffer, count,
                                                                             datatype, root,
                                                                             comm_ptr, 1, sched);
                    break;

                case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_tsp_ring:
                    mpi_errno =
                        MPIR_TSP_Ibcast_sched_intra_tree(buffer, count, datatype, root, comm_ptr,
                                                         MPIR_TREE_TYPE_KARY, 1,
                                                         cnt->u.ibcast.intra_tsp_tree.chunk_size,
                                                         sched);
                    break;
                default:
                    /* Replace this call with MPIR_Assert(0) when json files have gentran algos */
                    goto fallback;
                    break;
            }
    }
    MPIR_ERR_CHECK(mpi_errno);
    goto fn_exit;

  fallback:
    mpi_errno = MPIR_Ibcast_sched_intra_tsp_flat_auto(buffer, count, datatype, root,
                                                      comm_ptr, sched);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
