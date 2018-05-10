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

/* Header protection (i.e., IBCAST_TSP_RING_ALGOS_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "algo_common.h"
#include "tsp_namespace_def.h"

/* Routine to schedule a pipelined ring based broadcast */
#undef FUNCNAME
#define FUNCNAME MPIR_TSP_Ibcast_sched_intra_ring
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_TSP_Ibcast_sched_intra_ring(void *buffer, int count, MPI_Datatype datatype, int root,
                                     MPIR_Comm * comm, int maxbytes, MPIR_TSP_sched_t * sched)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    int num_chunks, chunk_size_floor, chunk_size_ceil;
    int offset = 0;
    size_t extent, type_size;
    MPI_Aint lb, true_extent;
    int size;
    int rank;
    int recv_id;
    int tag;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IBCAST_SCHED_INTRA_RING);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IBCAST_SCHED_INTRA_RING);

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "Scheduling pipelined ring broadcast on %d ranks, root=%d\n",
                     MPIR_Comm_size(comm), root));

    size = MPIR_Comm_size(comm);
    rank = MPIR_Comm_rank(comm);

    MPIR_Datatype_get_size_macro(datatype, type_size);
    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &lb, &true_extent);
    extent = MPL_MAX(extent, true_extent);

    /* calculate chunking information for pipelining */
    MPII_Algo_calculate_pipeline_chunk_info(maxbytes, type_size, count, &num_chunks,
                                            &chunk_size_floor, &chunk_size_ceil);
    /* print chunking information */
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST,
                                             "Broadcast pipeline info: maxbytes=%d count=%d num_chunks=%d chunk_size_floor=%d chunk_size_ceil=%d \n",
                                             maxbytes, count, num_chunks,
                                             chunk_size_floor, chunk_size_ceil));

    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    mpi_errno = MPIR_Sched_next_tag(comm, &tag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* do pipelined ring broadcast */
    /* NOTE: Make sure you are handling non-contiguous datatypes
     * correctly with pipelined broadcast, for example, buffer+offset
     * if being calculated correctly */
    for (i = 0; i < num_chunks; i++) {
        int msgsize = (i == 0) ? chunk_size_floor : chunk_size_ceil;

        /* Receive message from predecessor */
        if (rank != 0) {
            recv_id =
                MPIR_TSP_sched_irecv((char *) buffer + offset * extent, msgsize, datatype,
                                     rank - 1, tag, comm, sched, 0, NULL);
        }

        /* Send data to successor */
        if (rank != size - 1) {
            MPIR_TSP_sched_isend((char *) buffer + offset * extent, msgsize, datatype,
                                 rank + 1, tag, comm, sched, (rank == 0) ? 0 : 1, &recv_id);
        }
        offset += msgsize;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IBCAST_SCHED_INTRA_RING);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


/* Non-blocking ring based broadcast */
#undef FUNCNAME
#define FUNCNAME MPIR_TSP_Ibcast_intra_ring
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_TSP_Ibcast_intra_ring(void *buffer, int count, MPI_Datatype datatype, int root,
                               MPIR_Comm * comm, MPIR_Request ** req, int maxbytes)
{
    int mpi_errno = MPI_SUCCESS;
    int tag;
    MPIR_TSP_sched_t *sched;
    *req = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IBCAST_INTRA_RING);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IBCAST_INTRA_RING);

    /* generate the schedule */
    sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
    MPIR_TSP_sched_create(sched);

    /* schedule pipelined ring algo */
    mpi_errno = MPIR_TSP_Ibcast_sched_intra_ring(buffer, count, datatype, root, comm,
                                                 maxbytes, sched);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* start and register the schedule */
    mpi_errno = MPIR_TSP_sched_start(sched, comm, req);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IBCAST_INTRA_RING);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
