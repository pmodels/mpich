/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "algo_common.h"
#include "treealgo.h"

/* Routine to schedule a pipelined tree based allreduce */
int MPIR_TSP_Iallreduce_sched_intra_tsp_auto(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                             MPI_Datatype datatype, MPI_Op op,
                                             MPIR_Comm * comm, MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    int is_commutative = MPIR_Op_is_commutative(op);
    int nranks = comm->local_size;

    MPIR_Assert(comm->comm_kind == MPIR_COMM_KIND__INTRACOMM);

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__IALLREDUCE,
        .comm_ptr = comm,

        .u.iallreduce.sendbuf = sendbuf,
        .u.iallreduce.recvbuf = recvbuf,
        .u.iallreduce.count = count,
        .u.iallreduce.datatype = datatype,
        .u.iallreduce.op = op,
    };

    MPII_Csel_container_s *cnt;

    switch (MPIR_CVAR_IALLREDUCE_INTRA_ALGORITHM) {
        case MPIR_CVAR_IALLREDUCE_INTRA_ALGORITHM_tsp_recexch_single_buffer:
            mpi_errno =
                MPIR_TSP_Iallreduce_sched_intra_recexch(sendbuf, recvbuf, count,
                                                        datatype, op, comm,
                                                        MPIR_IALLREDUCE_RECEXCH_TYPE_SINGLE_BUFFER,
                                                        MPIR_CVAR_IALLREDUCE_RECEXCH_KVAL, sched);
            break;

        case MPIR_CVAR_IALLREDUCE_INTRA_ALGORITHM_tsp_recexch_multiple_buffer:
            mpi_errno =
                MPIR_TSP_Iallreduce_sched_intra_recexch(sendbuf, recvbuf, count,
                                                        datatype, op, comm,
                                                        MPIR_IALLREDUCE_RECEXCH_TYPE_MULTIPLE_BUFFER,
                                                        MPIR_CVAR_IALLREDUCE_RECEXCH_KVAL, sched);
            break;

        case MPIR_CVAR_IALLREDUCE_INTRA_ALGORITHM_tsp_tree:
            /*Only knomial_1 tree supports non-commutative operations */
            MPII_COLLECTIVE_FALLBACK_CHECK(comm->rank, is_commutative ||
                                           MPIR_Iallreduce_tree_type ==
                                           MPIR_TREE_TYPE_KNOMIAL_1, mpi_errno,
                                           "Iallreduce gentran_tree cannot be applied.\n");
            mpi_errno =
                MPIR_TSP_Iallreduce_sched_intra_tree(sendbuf, recvbuf, count, datatype, op,
                                                     comm, MPIR_Iallreduce_tree_type,
                                                     MPIR_CVAR_IALLREDUCE_TREE_KVAL,
                                                     MPIR_CVAR_IALLREDUCE_TREE_PIPELINE_CHUNK_SIZE,
                                                     MPIR_CVAR_IALLREDUCE_TREE_BUFFER_PER_CHILD,
                                                     sched);
            break;

        case MPIR_CVAR_IALLREDUCE_INTRA_ALGORITHM_tsp_ring:
            MPII_COLLECTIVE_FALLBACK_CHECK(comm->rank, is_commutative, mpi_errno,
                                           "Iallreduce gentran_ring cannot be applied.\n");
            mpi_errno =
                MPIR_TSP_Iallreduce_sched_intra_ring(sendbuf, recvbuf, count, datatype,
                                                     op, comm, sched);
            break;
        case MPIR_CVAR_IALLREDUCE_INTRA_ALGORITHM_tsp_recexch_reduce_scatter_recexch_allgatherv:
            /* This algorithm will work for commutative
             * operations and if the count is bigger than total
             * number of ranks. If it not commutative or if the
             * count < nranks, MPIR_Iallreduce_sched algorithm
             * will be run */
            MPII_COLLECTIVE_FALLBACK_CHECK(comm->rank, is_commutative &&
                                           count >= nranks, mpi_errno,
                                           "Iallreduce gentran_recexch_reduce_scatter_recexch_allgatherv cannot be applied.\n");
            mpi_errno =
                MPIR_TSP_Iallreduce_sched_intra_recexch_reduce_scatter_recexch_allgatherv(sendbuf,
                                                                                          recvbuf,
                                                                                          count,
                                                                                          datatype,
                                                                                          op,
                                                                                          comm,
                                                                                          MPIR_CVAR_IALLREDUCE_RECEXCH_KVAL,
                                                                                          sched);
            break;
        default:
            cnt = MPIR_Csel_search(comm->csel_comm, coll_sig);
            MPIR_Assert(cnt);

            switch (cnt->id) {
                case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_tsp_recexch_single_buffer:
                    mpi_errno =
                        MPIR_TSP_Iallreduce_sched_intra_recexch(sendbuf, recvbuf, count,
                                                                datatype, op, comm,
                                                                MPIR_IALLREDUCE_RECEXCH_TYPE_SINGLE_BUFFER,
                                                                cnt->u.
                                                                iallreduce.intra_tsp_recexch_single_buffer.
                                                                k, sched);
                    break;

                case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_tsp_recexch_multiple_buffer:
                    mpi_errno =
                        MPIR_TSP_Iallreduce_sched_intra_recexch(sendbuf, recvbuf, count,
                                                                datatype, op, comm,
                                                                MPIR_IALLREDUCE_RECEXCH_TYPE_MULTIPLE_BUFFER,
                                                                cnt->u.
                                                                iallreduce.intra_tsp_recexch_single_buffer.
                                                                k, sched);
                    break;

                case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_tsp_tree:
                    mpi_errno =
                        MPIR_TSP_Iallreduce_sched_intra_tree(sendbuf, recvbuf, count, datatype, op,
                                                             comm,
                                                             cnt->u.iallreduce.
                                                             intra_tsp_tree.tree_type,
                                                             cnt->u.iallreduce.intra_tsp_tree.k,
                                                             cnt->u.iallreduce.
                                                             intra_tsp_tree.chunk_size,
                                                             cnt->u.iallreduce.
                                                             intra_tsp_tree.buffer_per_child,
                                                             sched);
                    break;

                case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_tsp_ring:
                    mpi_errno =
                        MPIR_TSP_Iallreduce_sched_intra_ring(sendbuf, recvbuf, count, datatype, op,
                                                             comm, sched);
                    break;

                case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_tsp_recexch_reduce_scatter_recexch_allgatherv:
                    mpi_errno =
                        MPIR_TSP_Iallreduce_sched_intra_recexch_reduce_scatter_recexch_allgatherv
                        (sendbuf, recvbuf, count, datatype, op, comm,
                         cnt->u.iallreduce.intra_tsp_recexch_reduce_scatter_recexch_allgatherv.k,
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
    mpi_errno =
        MPIR_TSP_Iallreduce_sched_intra_recexch(sendbuf, recvbuf, count,
                                                datatype, op, comm,
                                                MPIR_IALLREDUCE_RECEXCH_TYPE_MULTIPLE_BUFFER,
                                                MPIR_CVAR_IALLREDUCE_RECEXCH_KVAL, sched);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
