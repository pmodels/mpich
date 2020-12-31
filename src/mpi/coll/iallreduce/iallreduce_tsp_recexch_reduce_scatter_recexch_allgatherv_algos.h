/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* Header protection (i.e., IALLREDUCE_TSP_ALGOS_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "algo_common.h"
#include "tsp_namespace_def.h"
#include "iallreduce_tsp_recursive_exchange_common_prototypes.h"
#include "../iallgatherv/iallgatherv_tsp_recexch_algos_prototypes.h"
#include "../ireduce_scatter/ireduce_scatter_tsp_recexch_algos_prototypes.h"

/* Routine to schedule a recursive exchange based allreduce */
int MPIR_TSP_Iallreduce_sched_intra_recexch_reduce_scatter_recexch_allgatherv(const void *sendbuf,
                                                                              void *recvbuf,
                                                                              int count,
                                                                              MPI_Datatype datatype,
                                                                              MPI_Op op,
                                                                              MPIR_Comm * comm,
                                                                              int k,
                                                                              MPIR_TSP_sched_t *
                                                                              sched)
{
    int mpi_errno = MPI_SUCCESS;
    int is_inplace, i;
    int dtcopy_id = -1;
    size_t extent;
    MPI_Aint lb, true_extent;
    int is_commutative;
    int nranks, rank;
    int step1_sendto = -1, step1_nrecvs, *step1_recvfrom = NULL;
    int step2_nphases = 0, **step2_nbrs = NULL;
    int p_of_k, log_pofk, T;
    int per_nbr_buffer = 0, rem = 0;
    int nvtcs, sink_id, *recv_id = NULL, *vtcs = NULL;
    int *send_id = NULL, *reduce_id = NULL, *cnts = NULL, *displs = NULL;
    bool in_step2 = false;
    void *tmp_recvbuf;
    void **step1_recvbuf = NULL;
    int tag;
    int allgather_algo_type = MPIR_IALLGATHER_RECEXCH_TYPE_DISTANCE_HALVING;
    int redscat_algo_type = IREDUCE_SCATTER_RECEXCH_TYPE_DISTANCE_HALVING;
    MPIR_CHKLMEM_DECL(2);

    MPIR_FUNC_VERBOSE_STATE_DECL
        (MPID_STATE_MPIR_TSP_IALLREDUCE_SCHED_INTRA_RECEXCH_REDUCE_SCATTER_RECEXCH_ALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER
        (MPID_STATE_MPIR_TSP_IALLREDUCE_SCHED_INTRA_RECEXCH_REDUCE_SCATTER_RECEXCH_ALLGATHERV);

    is_inplace = (sendbuf == MPI_IN_PLACE);
    nranks = MPIR_Comm_size(comm);
    rank = MPIR_Comm_rank(comm);

    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &lb, &true_extent);
    extent = MPL_MAX(extent, true_extent);
    is_commutative = MPIR_Op_is_commutative(op);

    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    mpi_errno = MPIR_Sched_next_tag(comm, &tag);
    MPIR_ERR_CHECK(mpi_errno);

    /* if there is only 1 rank, copy data from sendbuf
     * to recvbuf and exit */
    if (nranks == 1) {
        if (!is_inplace && count > 0)
            MPIR_TSP_sched_localcopy(sendbuf, count, datatype, recvbuf, count, datatype, sched, 0,
                                     NULL);
        return mpi_errno;
    }

    /* get the neighbors, the function allocates the required memory */
    MPII_Recexchalgo_get_neighbors(rank, nranks, &k, &step1_sendto,
                                   &step1_recvfrom, &step1_nrecvs,
                                   &step2_nbrs, &step2_nphases, &p_of_k, &T);
    in_step2 = (step1_sendto == -1);    /* whether this rank participates in Step 2 */
    log_pofk = step2_nphases;
    send_id = (int *) MPL_malloc(sizeof(int) * k, MPL_MEM_COLL);        /* to store send vertex ids */
    reduce_id = (int *) MPL_malloc(sizeof(int) * k, MPL_MEM_COLL);      /* to store reduce vertex ids */
    recv_id = (int *) MPL_malloc(sizeof(int) * ((step2_nphases * (k - 1)) + 1), MPL_MEM_COLL);  /* to store receive vertex ids */
    vtcs = MPL_malloc(sizeof(int) * (step2_nphases) * k, MPL_MEM_COLL);
    tmp_recvbuf = MPIR_TSP_sched_malloc(count * extent, sched);


    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "Beforeinitial dt copy\n"));
    if (in_step2 && !is_inplace && count > 0) { /* copy the data to tmp_recvbuf but only if you are a rank participating in Step 2 */
        dtcopy_id = MPIR_TSP_sched_localcopy(sendbuf, count, datatype,
                                             recvbuf, count, datatype, sched, 0, NULL);
    }
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "After initial dt copy\n"));

    per_nbr_buffer = 0;
    /* Step 1 */
    MPIR_TSP_Iallreduce_sched_intra_recexch_step1(sendbuf, recvbuf, count,
                                                  datatype, op, is_commutative,
                                                  tag, extent, dtcopy_id, recv_id, reduce_id, vtcs,
                                                  is_inplace, step1_sendto, in_step2, step1_nrecvs,
                                                  step1_recvfrom, per_nbr_buffer, &step1_recvbuf,
                                                  comm, sched);

    sink_id = MPIR_TSP_sched_sink(sched);       /* sink for all the tasks up to end of Step 1 */

    /* Step 2 */
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "Start Step2"));

    if (in_step2) {
        MPIR_CHKLMEM_MALLOC(cnts, int *, sizeof(int) * nranks, mpi_errno, "cnts", MPL_MEM_COLL);
        MPIR_CHKLMEM_MALLOC(displs, int *, sizeof(int) * nranks, mpi_errno, "displs", MPL_MEM_COLL);
        int idx = 0;
        rem = nranks - p_of_k;

        for (i = 0; i < nranks; i++)
            cnts[i] = 0;
        for (i = 0; i < (p_of_k - 1); i++) {
            idx = (i < rem / (k - 1)) ? (i * k) + (k - 1) : i + rem;
            cnts[idx] = count / p_of_k;
        }
        idx = (p_of_k - 1 < rem / (k - 1)) ? (p_of_k - 1 * k) + (k - 1) : p_of_k - 1 + rem;
        cnts[idx] = count - (count / p_of_k) * (p_of_k - 1);

        displs[0] = 0;
        for (i = 1; i < nranks; i++) {
            displs[i] = displs[i - 1] + cnts[i - 1];
        }

        MPIR_TSP_Ireduce_scatter_sched_intra_recexch_step2(recvbuf, tmp_recvbuf,
                                                           cnts, displs, datatype, op, extent, tag,
                                                           comm, k, redscat_algo_type,
                                                           step2_nphases, step2_nbrs, rank, nranks,
                                                           sink_id, 0, NULL, sched);

        MPIR_TSP_sched_fence(sched);    /* sink for all the tasks up till this point */

        MPIR_TSP_Iallgatherv_sched_intra_recexch_step2(step1_sendto, step2_nphases, step2_nbrs,
                                                       rank, nranks, k, p_of_k, log_pofk, T, &nvtcs,
                                                       &recv_id, tag, recvbuf, extent, cnts, displs,
                                                       datatype, allgather_algo_type, comm, sched);

    }

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "After Step 2\n"));

    /* Step 3: This is reverse of Step 1. Ranks that participated in Step 2
     * send the data to non-partcipating ranks */
    if (step1_sendto != -1) {   /* I am a Step 2 non-participating rank */
        MPIR_TSP_sched_irecv(recvbuf, count, datatype, step1_sendto, tag, comm, sched, 1, &sink_id);
    } else {
        for (i = 0; i < step1_nrecvs; i++) {
            MPIR_TSP_sched_isend(recvbuf, count, datatype, step1_recvfrom[i], tag, comm, sched,
                                 nvtcs, recv_id);
        }
    }

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "Done Step 3\n"));

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    /* free all allocated memory for storing nbrs */
    for (i = 0; i < step2_nphases; i++)
        MPL_free(step2_nbrs[i]);
    MPL_free(step2_nbrs);
    MPL_free(step1_recvfrom);
    MPL_free(send_id);
    MPL_free(reduce_id);
    MPL_free(recv_id);
    MPL_free(vtcs);
    if (in_step2)
        MPL_free(step1_recvbuf);

    MPIR_FUNC_VERBOSE_EXIT
        (MPID_STATE_MPIR_TSP_IALLREDUCE_SCHED_INTRA_RECEXCH_REDUCE_SCATTER_RECEXCH_ALLGATHERV);

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


/* Non-blocking recexch_reduce_scatter_recexch_allgatherv based ALLREDUCE */
int MPIR_TSP_Iallreduce_intra_recexch_reduce_scatter_recexch_allgatherv(const void *sendbuf,
                                                                        void *recvbuf, int count,
                                                                        MPI_Datatype datatype,
                                                                        MPI_Op op, MPIR_Comm * comm,
                                                                        MPIR_Request ** req, int k)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_TSP_sched_t *sched;
    *req = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL
        (MPID_STATE_MPIR_TSP_IALLREDUCE_INTRA_RECEXCH_REDUCE_SCATTER_RECEXCH_ALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER
        (MPID_STATE_MPIR_TSP_IALLREDUCE_INTRA_RECEXCH_REDUCE_SCATTER_RECEXCH_ALLGATHERV);


    /* generate the schedule */
    sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
    MPIR_Assert(sched != NULL);
    MPIR_TSP_sched_create(sched, false);

    mpi_errno =
        MPIR_TSP_Iallreduce_sched_intra_recexch_reduce_scatter_recexch_allgatherv(sendbuf, recvbuf,
                                                                                  count, datatype,
                                                                                  op, comm, k,
                                                                                  sched);

    MPIR_ERR_CHECK(mpi_errno);

    /* start and register the schedule */
    mpi_errno = MPIR_TSP_sched_start(sched, comm, req);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT
        (MPID_STATE_MPIR_TSP_IALLREDUCE_INTRA_RECEXCH_REDUCE_SCATTER_RECEXCH_ALLGATHERV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
