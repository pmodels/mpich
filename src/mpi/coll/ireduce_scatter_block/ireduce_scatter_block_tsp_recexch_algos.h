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

/* Header protection (i.e., IREDUCE_SCATTER_BLOCK_TSP_RECEXCH_ALGOS_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "algo_common.h"
#include "tsp_namespace_def.h"
#include "recexchalgo.h"

/* Routine to schedule a recursive exchange based reduce_scatter with distance halving in each phase */
#undef FUNCNAME
#define FUNCNAME MPIR_TSP_Ireduce_scatter_block_sched_intra_recexch
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_TSP_Ireduce_scatter_block_sched_intra_recexch(const void *sendbuf, void *recvbuf,
                                                       int recvcount, MPI_Datatype datatype,
                                                       MPI_Op op, int tag, MPIR_Comm * comm, int k,
                                                       MPIR_TSP_sched_t * sched)
{
    int mpi_errno = MPI_SUCCESS;
    int is_inplace;
    size_t extent;
    MPI_Aint lb, true_extent;
    int is_commutative;
    int step1_sendto = -1, step2_nphases = 0, step1_nrecvs = 0;
    int in_step2;
    int *step1_recvfrom = NULL;
    int **step2_nbrs = NULL;
    int nranks, rank, p_of_k, T, dst;
    int total_count, send_cnt, recv_cnt;
    int i, phase, offset, send_offset, recv_offset;
    int dtcopy_id = -1, send_id = -1, recv_id = -1, reduce_id = -1, step1_id = -1;
    int nvtcs, vtcs[2];
    void *tmp_recvbuf = NULL, *tmp_results = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IREDUCE_SCATTER_BLOCK_SCHED_INTRA_RECEXCH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IREDUCE_SCATTER_BLOCK_SCHED_INTRA_RECEXCH);

    if (recvcount == 0) {
        return mpi_errno;
    }

    is_inplace = (sendbuf == MPI_IN_PLACE);
    nranks = MPIR_Comm_size(comm);
    rank = MPIR_Comm_rank(comm);

    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &lb, &true_extent);
    extent = MPL_MAX(extent, true_extent);
    is_commutative = MPIR_Op_is_commutative(op);
    MPIR_Assert(is_commutative == 1);

    total_count = nranks * recvcount;

    /* if there is only 1 rank, copy data from sendbuf
     * to recvbuf and exit */
    if (nranks == 1) {
        if (!is_inplace)
            MPIR_TSP_sched_localcopy(sendbuf, total_count, datatype, recvbuf, total_count,
                                     datatype, sched, 0, NULL);
        return mpi_errno;
    }

    /* get the neighbors, the function allocates the required memory */
    MPII_Recexchalgo_get_neighbors(rank, nranks, &k, &step1_sendto,
                                   &step1_recvfrom, &step1_nrecvs,
                                   &step2_nbrs, &step2_nphases, &p_of_k, &T);
    in_step2 = (step1_sendto == -1);    /* whether this rank participates in Step 2 */
    tmp_results = MPIR_TSP_sched_malloc(total_count * extent, sched);
    tmp_recvbuf = MPIR_TSP_sched_malloc(total_count * extent, sched);


    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "Beforeinitial dt copy\n"));

    if (in_step2) {
        if (!is_inplace)
            dtcopy_id = MPIR_TSP_sched_localcopy(sendbuf, total_count, datatype,
                                                 tmp_results, total_count, datatype, sched, 0,
                                                 NULL);
        else
            dtcopy_id = MPIR_TSP_sched_localcopy(recvbuf, total_count, datatype,
                                                 tmp_results, total_count, datatype, sched, 0,
                                                 NULL);
    }
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "After initial dt copy\n"));

    /* Step 1 */
    if (!in_step2) {
        /* non-participating rank sends the data to a participating rank */
        void *buf_to_send;
        if (is_inplace)
            buf_to_send = recvbuf;
        else
            buf_to_send = (void *) sendbuf;
        MPIR_TSP_sched_isend(buf_to_send, total_count, datatype, step1_sendto, tag, comm, sched, 0,
                             NULL);
    } else {    /* Step 2 participating rank */
        for (i = 0; i < step1_nrecvs; i++) {    /* participating rank gets data from non-partcipating ranks */
            /* recv dependencies */
            nvtcs = 1;
            vtcs[0] = (i == 0) ? dtcopy_id : reduce_id;
            recv_id = MPIR_TSP_sched_irecv(tmp_recvbuf, total_count, datatype,
                                           step1_recvfrom[i], tag, comm, sched, nvtcs, vtcs);

            nvtcs++;
            vtcs[1] = recv_id;
            reduce_id = MPIR_TSP_sched_reduce_local(tmp_recvbuf, tmp_results, total_count,
                                                    datatype, op, sched, nvtcs, vtcs);
        }
    }

    step1_id = MPIR_TSP_sched_sink(sched);      /* sink for all the tasks up to end of Step 1 */

    /* Step 2 */
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "Start Step2"));

    for (phase = step2_nphases - 1; phase >= 0 && step1_sendto == -1; phase--) {
        for (i = 0; i < k - 1; i++) {
            dst = step2_nbrs[phase][i];
            send_cnt = recv_cnt = 0;

            /* Both send and recv have similar dependencies */
            nvtcs = 1;
            if (phase == step2_nphases - 1 && i == 0) {
                vtcs[0] = step1_id;
            } else {
                vtcs[0] = reduce_id;
            }

            MPII_Recexchalgo_get_count_and_offset(dst, phase, k, nranks, &send_cnt, &offset);
            send_offset = offset * extent * recvcount;
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST,
                             "phase %d sending to %d send_offset %d send_count %d \n", phase, dst,
                             send_offset, send_cnt * recvcount));

            send_id =
                MPIR_TSP_sched_isend((char *) tmp_results + send_offset, send_cnt * recvcount,
                                     datatype, dst, tag, comm, sched, nvtcs, vtcs);

            MPII_Recexchalgo_get_count_and_offset(rank, phase, k, nranks, &recv_cnt, &offset);
            recv_offset = offset * extent * recvcount;
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST,
                             "phase %d recving from %d recv_offset %d recv_count %d \n", phase, dst,
                             recv_offset, recv_cnt * recvcount));

            recv_id =
                MPIR_TSP_sched_irecv(tmp_recvbuf, recv_cnt * recvcount,
                                     datatype, dst, tag, comm, sched, nvtcs, vtcs);

            nvtcs = 2;
            vtcs[0] = send_id;
            vtcs[1] = recv_id;
            reduce_id =
                MPIR_TSP_sched_reduce_local(tmp_recvbuf,
                                            (char *) tmp_results + recv_offset,
                                            recv_cnt * recvcount, datatype, op, sched, nvtcs, vtcs);
        }
    }
    if (in_step2) {
        nvtcs = 1;
        vtcs[0] = reduce_id;
        /* copy data from tmp_results buffer correct position into recvbuf for all participating ranks */
        dtcopy_id =
            MPIR_TSP_sched_localcopy((char *) tmp_results + rank * recvcount * extent, recvcount,
                                     datatype, recvbuf, recvcount, datatype, sched, nvtcs, vtcs);
    }
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "After Step 2\n"));

    /* Step 3: This is reverse of Step 1. Ranks that participated in Step 2
     * send the data to non-partcipating ranks */
    if (step1_sendto != -1) {   /* I am a Step 2 non-participating rank */
        MPIR_TSP_sched_irecv(recvbuf, recvcount, datatype, step1_sendto, tag, comm, sched, 1,
                             &step1_id);
    }
    for (i = 0; i < step1_nrecvs; i++) {
        nvtcs = 1;
        vtcs[0] = reduce_id;
        MPIR_TSP_sched_isend((char *) tmp_results + recvcount * step1_recvfrom[i] * extent,
                             recvcount, datatype, step1_recvfrom[i], tag, comm, sched, nvtcs, vtcs);
    }

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "Done Step 3\n"));

  fn_exit:
    /* free all allocated memory for storing nbrs */
    for (i = 0; i < step2_nphases; i++)
        MPL_free(step2_nbrs[i]);
    MPL_free(step2_nbrs);
    MPL_free(step1_recvfrom);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IREDUCE_SCATTER_BLOCK_SCHED_INTRA_RECEXCH);

    return mpi_errno;
}


/* Non-blocking recexch based REDUCE_SCATTER_BLOCK */
#undef FUNCNAME
#define FUNCNAME MPIR_TSP_Ireduce_scatter_block_intra_recexch
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_TSP_Ireduce_scatter_block_intra_recexch(const void *sendbuf, void *recvbuf, int recvcount,
                                                 MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                                 MPIR_Request ** req, int k)
{
    int mpi_errno = MPI_SUCCESS;
    int tag;
    MPIR_TSP_sched_t *sched;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IREDUCE_SCATTER_BLOCK_INTRA_RECEXCH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IREDUCE_SCATTER_BLOCK_INTRA_RECEXCH);

    *req = NULL;

    /* generate the schedule */
    sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
    MPIR_Assert(sched != NULL);
    MPIR_TSP_sched_create(sched);

    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    mpi_errno = MPIR_Sched_next_tag(comm, &tag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno =
        MPIR_TSP_Ireduce_scatter_block_sched_intra_recexch(sendbuf, recvbuf, recvcount, datatype,
                                                           op, tag, comm, k, sched);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* start and register the schedule */
    mpi_errno = MPIR_TSP_sched_start(sched, comm, req);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IREDUCE_SCATTER_BLOCK_INTRA_RECEXCH);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
