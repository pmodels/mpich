/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

/* Header protection (i.e., IREDUCE_TSP_RING_ALGOS_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "algo_common.h"
#include "tsp_namespace_def.h"

/* Routine to schedule a pipelined ring based reduce */
#undef FUNCNAME
#define FUNCNAME MPIR_TSP_Ireduce_sched_intra_ring
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_TSP_Ireduce_sched_intra_ring(const void *sendbuf, void *recvbuf, int count,
                                      MPI_Datatype datatype, MPI_Op op, int root,
                                      MPIR_Comm * comm, int maxbytes, MPIR_TSP_sched_t * sched)
{
    int mpi_errno = MPI_SUCCESS;
    int j;
    int num_chunks, chunk_size_floor, chunk_size_ceil;
    int offset = 0;
    size_t extent, type_size;
    MPI_Aint type_lb, true_extent;
    int is_commutative;
    int size;
    int rank;
    int ring_root;              /* Root of the chain over which reduction is performed.
                                 * This can be different from root of the collective operation.
                                 * When root is non-zero and op is non-commutative, the reduction
                                 * is first performed with rank 0 as the root of the ring and then
                                 * rank 0 sends the reduced data to the root of the collective op. */
    int is_ring_root, is_ring_leaf, is_ring_intermediate;       /* Variables to store location of this rank in the ring */
    int is_root;
    void *recv_buffer;          /* Buffer in which data from neighboring rank is received */
    void *reduce_buffer;        /* Buffer in which reduced data is present */
    int recv_id, reduce_id;     /* Variables to store graph vertex ids */
    int tag;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IREDUCE_SCHED_INTRA_RING);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IREDUCE_SCHED_INTRA_RING);

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "Scheduling pipelined ring reduce on %d ranks, root=%d\n",
                     MPIR_Comm_size(comm), root));

    size = MPIR_Comm_size(comm);
    rank = MPIR_Comm_rank(comm);
    is_root = (rank == root);

    MPIR_Datatype_get_size_macro(datatype, type_size);
    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &type_lb, &true_extent);
    extent = MPL_MAX(extent, true_extent);
    is_commutative = MPIR_Op_is_commutative(op);

    /* calculate chunking information for pipelining */
    MPII_Algo_calculate_pipeline_chunk_info(maxbytes, type_size, count, &num_chunks,
                                            &chunk_size_floor, &chunk_size_ceil);
    /* print chunking information */
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST,
                                             "Reduce pipeline info: maxbytes=%d count=%d num_chunks=%d chunk_size_floor=%d chunk_size_ceil=%d \n",
                                             maxbytes, count, num_chunks,
                                             chunk_size_floor, chunk_size_ceil));

    if (!is_commutative) {
        ring_root = 0;  /* Force the ring root to be rank 0 */
    } else {
        ring_root = root;
    }

    /* identify my locaion in the ring */
    is_ring_root = (rank == ring_root) ? 1 : 0;
    is_ring_leaf = ((ring_root + size - 1) % size == rank) ? 1 : 0;
    is_ring_intermediate = (!is_ring_leaf && !is_ring_root);

    /* allocate buffers to receive data from neighbor */
    if (!is_ring_leaf) {
        recv_buffer = MPIR_TSP_sched_malloc(extent * count, sched);
        recv_buffer = (void *) ((char *) recv_buffer - type_lb);
    }

    /* Set reduce_buffer based on location in the ring */
    /* FIXME: This can also be pipelined along with rest of the reduction */
    if (is_ring_root && is_root) {
        reduce_buffer = recvbuf;
        if (sendbuf != MPI_IN_PLACE)
            MPIR_Localcopy(sendbuf, count, datatype, recvbuf, count, datatype);
    } else if (is_ring_root && !is_root) {
        reduce_buffer = MPIR_TSP_sched_malloc(extent * count, sched);
        reduce_buffer = (void *) ((char *) reduce_buffer - type_lb);
        MPIR_Localcopy(sendbuf, count, datatype, reduce_buffer, count, datatype);
    } else if (is_ring_intermediate && is_root) {
        reduce_buffer = recvbuf;
        if (sendbuf != MPI_IN_PLACE)
            MPIR_Localcopy(sendbuf, count, datatype, recvbuf, count, datatype);
    } else if (is_ring_intermediate && !is_root) {
        reduce_buffer = MPIR_TSP_sched_malloc(extent * count, sched);
        reduce_buffer = (void *) ((char *) reduce_buffer - type_lb);
        MPIR_Localcopy(sendbuf, count, datatype, reduce_buffer, count, datatype);
    } else if (is_ring_leaf && is_root) {
        if (sendbuf == MPI_IN_PLACE)
            reduce_buffer = recvbuf;
        else
            reduce_buffer = (void *) sendbuf;
    } else if (is_ring_leaf && !is_root) {
        reduce_buffer = (void *) sendbuf;
    }

    /* do pipelined reduce */
    /* NOTE: Make sure you are handling non-contiguous datatypes
     * correctly with pipelined reduce, for example, buffer+offset
     * if being calculated correctly */
    for (j = 0; j < num_chunks; j++) {
        int msgsize = (j == 0) ? chunk_size_floor : chunk_size_ceil;
        void *reduce_address = (char *) reduce_buffer + offset * extent;

        /* For correctness, transport based collectives need to get the
         * tag from the same pool as schedule based collectives */
        mpi_errno = MPIR_Sched_next_tag(comm, &tag);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        if (!is_ring_leaf) {
            void *recv_address = (char *) recv_buffer + offset * extent;
            int src = (rank + 1) % size;

            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST, "Schedule receive from rank %d\n", src));
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST, "Posting receive at address %p\n", recv_address));

            recv_id = MPIR_TSP_sched_irecv(recv_address, msgsize, datatype, src, tag, comm,
                                           sched, 0, NULL);

            if (is_commutative) {       /* reduction order does not matter */
                reduce_id = MPIR_TSP_sched_reduce_local(recv_address, reduce_address, msgsize,
                                                        datatype, op, sched, 1, &recv_id);
            } else {    /* wait for the previous reduce to complete */

                reduce_id =
                    MPIR_TSP_sched_reduce_local(reduce_address, recv_address, msgsize, datatype, op,
                                                sched, 1, &recv_id);
                reduce_id =
                    MPIR_TSP_sched_localcopy(recv_address, msgsize, datatype, reduce_address,
                                             msgsize, datatype, sched, 1, &reduce_id);
            }
        }

        /* send data to the parent */
        if (!is_ring_root)
            MPIR_TSP_sched_isend(reduce_address, msgsize, datatype, (rank - 1 + size) % size, tag,
                                 comm, sched, (is_ring_leaf) ? 0 : 1, &reduce_id);

        /* send data to the root of the collective operation */
        if (ring_root != root) {
            if (is_ring_root) { /* ring_root sends data to root */
                MPIR_TSP_sched_isend(reduce_address, msgsize, datatype, root, tag, comm, sched,
                                     1, &reduce_id);
            } else if (is_root) {       /* root receives data from ring_root */
                MPIR_TSP_sched_irecv((char *) recvbuf + offset * extent, msgsize, datatype,
                                     ring_root, tag, comm, sched, 0, NULL);
            }
        }

        offset += msgsize;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IREDUCE_SCHED_INTRA_RING);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


/* Non-blocking ring based reduce */
#undef FUNCNAME
#define FUNCNAME MPIR_TSP_Ireduce_intra_ring
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_TSP_Ireduce_intra_ring(const void *sendbuf, void *recvbuf, int count,
                                MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm * comm,
                                MPIR_Request ** req, int maxbytes)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_TSP_sched_t *sched;
    *req = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IREDUCE_INTRA_RING);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IREDUCE_INTRA_RING);

    /* generate the schedule */
    sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
    MPIR_TSP_sched_create(sched);

    /* schedule pipelined tree algo */
    mpi_errno =
        MPIR_TSP_Ireduce_sched_intra_ring(sendbuf, recvbuf, count, datatype, op, root, comm,
                                          maxbytes, sched);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* start and register the schedule */
    mpi_errno = MPIR_TSP_sched_start(sched, comm, req);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IREDUCE_INTRA_RING);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
