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

/* Header protection (i.e., IBCAST_TSP_SCATTER_RECEXCH_ALLGATHER_ALGOS_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "algo_common.h"
#include "tsp_namespace_def.h"
#include "../iscatter/iscatter_tsp_tree_algos_prototypes.h"
#include "../iallgather/iallgather_tsp_recexch_algos_prototypes.h"

/* Routine to schedule a scatter followed by recursive exchange based broadcast */
#undef FUNCNAME
#define FUNCNAME MPIR_TSP_Ibcast_sched_intra_scatter_recexch_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_TSP_Ibcast_sched_intra_scatter_recexch_allgather(void *buffer, int count,
                                                          MPI_Datatype datatype, int root,
                                                          MPIR_Comm * comm,
                                                          MPIR_TSP_sched_t * sched)
{
    int mpi_errno = MPI_SUCCESS;
    size_t extent, type_size;
    MPI_Aint true_lb, true_extent;
    int size, rank;
    int is_contig;
    void *tmp_buf = NULL;
    size_t nbytes;
    int scatter_k = MPIR_CVAR_IBCAST_SCATTER_KVAL;
    int allgather_k = MPIR_CVAR_IBCAST_ALLGATHER_RECEXCH_KVAL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IBCAST_SCHED_INTRA_SCATTER_RECEXCH_ALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IBCAST_SCHED_INTRA_SCATTER_RECEXCH_ALLGATHER);

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST,
                     "Scheduling scatter followed by recursive exchange allgather based broadcast on %d ranks, root=%d\n",
                     MPIR_Comm_size(comm), root));

    size = MPIR_Comm_size(comm);
    rank = MPIR_Comm_rank(comm);

    MPIR_Datatype_get_size_macro(datatype, type_size);
    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
    MPIR_Datatype_is_contig(datatype, &is_contig);
    extent = MPL_MAX(extent, true_extent);

    nbytes = type_size * count;
    MPIR_Assert(nbytes % size == 0);

    if (is_contig) {
        /* contiguous. no need to pack. */
        tmp_buf = (char *) buffer + true_lb;
    } else {
        tmp_buf = (void *) MPIR_TSP_sched_malloc(nbytes, sched);

        if (rank == root) {
            mpi_errno =
                MPIR_TSP_sched_localcopy(buffer, count, datatype, tmp_buf, nbytes, MPI_BYTE, sched,
                                         0, NULL);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            MPIR_TSP_sched_fence(sched);
        }
    }

    /* Schedule scatter */
    int bytes_per_rank = nbytes / size;
    if (rank == root)
        MPIR_TSP_Iscatter_sched_intra_tree(tmp_buf, bytes_per_rank, MPI_BYTE, MPI_IN_PLACE, 0,
                                           MPI_DATATYPE_NULL, root, comm, scatter_k, sched);
    else
        MPIR_TSP_Iscatter_sched_intra_tree(NULL, 0, MPI_DATATYPE_NULL,
                                           ((char *) tmp_buf) + (rank * bytes_per_rank),
                                           bytes_per_rank, MPI_BYTE, root, comm, scatter_k, sched);
    MPIR_TSP_sched_fence(sched);        /* wait for scatter to complete */

    /* Schedule Allgather */
    MPIR_TSP_Iallgather_sched_intra_recexch(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, tmp_buf,
                                            bytes_per_rank, MPI_BYTE, comm, 0, allgather_k, sched);
    MPIR_TSP_sched_fence(sched);        /* wait for allgather to complete */

    if (!is_contig) {
        if (rank != root) {
            mpi_errno =
                MPIR_TSP_sched_localcopy(tmp_buf, nbytes, MPI_BYTE, buffer, count, datatype, sched,
                                         0, NULL);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IBCAST_SCHED_INTRA_SCATTER_RECEXCH_ALLGATHER);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


/* Non-blocking scatter followed by recursive exchange allgather  based broadcast */
#undef FUNCNAME
#define FUNCNAME MPIR_TSP_Ibcast_intra_ring
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_TSP_Ibcast_intra_scatter_recexch_allgather(void *buffer, int count, MPI_Datatype datatype,
                                                    int root, MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_TSP_sched_t *sched;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IBCAST_INTRA_SCATTER_RECEXCH_ALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IBCAST_INTRA_SCATTER_RECEXCH_ALLGATHER);

    *req = NULL;

    /* generate the schedule */
    sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
    MPIR_Assert(sched != NULL);
    MPIR_TSP_sched_create(sched);

    /* schedule scatter followed by recursive exchange allgather algo */
    mpi_errno =
        MPIR_TSP_Ibcast_sched_intra_scatter_recexch_allgather(buffer, count, datatype, root, comm,
                                                              sched);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* start and register the schedule */
    mpi_errno = MPIR_TSP_sched_start(sched, comm, req);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IBCAST_INTRA_SCATTER_RECEXCH_ALLGATHER);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
