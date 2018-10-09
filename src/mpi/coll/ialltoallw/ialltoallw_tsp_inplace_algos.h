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

/* Header protection (i.e., IALLTOALLW_TSP_ALGOS_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "algo_common.h"
#include "tsp_namespace_def.h"

/* Routine to schedule inplace algorithm for  alltoallw */
#undef FUNCNAME
#define FUNCNAME MPIR_TSP_Ialltoallw_sched_intra_inplace
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_TSP_Ialltoallw_sched_intra_inplace(const void *sendbuf, const int sendcounts[],
                                            const int sdispls[], const MPI_Datatype sendtypes[],
                                            void *recvbuf, const int recvcounts[],
                                            const int rdispls[], const MPI_Datatype recvtypes[],
                                            int tag, MPIR_Comm * comm, MPIR_Sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    int is_inplace;
    size_t recv_extent;
    MPI_Aint true_extent, true_lb;
    int nranks, rank, nvtcs;
    int i, bblock, comm_block, dst, send_id, recv_id, dtcopy_id = -1, vtcs[2];
    int max_size;
    void *tmp_buf = NULL, *adj_tmp_buf = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IALLTOALLV_SCHED_INTRA_INPLACE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IALLTOALLV_SCHED_INTRA_INPLACE);

    is_inplace = (sendbuf == MPI_IN_PLACE);
    nranks = MPIR_Comm_size(comm);
    rank = MPIR_Comm_rank(comm);

    max_size = 0;
    for (i = 0; i < nranks; ++i) {
        /* only look at recvtypes/recvcounts because the send vectors are
         * ignored when sendbuf==MPI_IN_PLACE */
        MPIR_Type_get_true_extent_impl(recvtypes[i], &true_lb, &true_extent);
        MPIR_Datatype_get_extent_macro(recvtypes[i], recv_extent);
        max_size = MPL_MAX(max_size, recvcounts[i] * MPL_MAX(recv_extent, true_extent));
    }

    tmp_buf = MPIR_TSP_sched_malloc(max_size, sched);

    for (i = 0; i < nranks; ++i) {
        if (rank != i) {
            dst = i;
            nvtcs = (dtcopy_id == -1) ? 0 : 1;
            vtcs[0] = dtcopy_id;

            MPIR_Type_get_true_extent_impl(recvtypes[i], &true_lb, &true_extent);
            adj_tmp_buf = (void *) ((char *) tmp_buf - true_lb);

            send_id = MPIR_TSP_sched_isend((char *) recvbuf + rdispls[dst],
                                           recvcounts[dst], recvtypes[dst], dst, tag, comm, sched,
                                           nvtcs, vtcs);

            recv_id =
                MPIR_TSP_sched_irecv(adj_tmp_buf, recvcounts[dst], recvtypes[dst], dst, tag, comm,
                                     sched, nvtcs, vtcs);

            nvtcs = 2;
            vtcs[0] = send_id;
            vtcs[1] = recv_id;
            dtcopy_id = MPIR_TSP_sched_localcopy(adj_tmp_buf, recvcounts[dst], recvtypes[dst],
                                                 ((char *) recvbuf + rdispls[dst]), recvcounts[dst],
                                                 recvtypes[dst], sched, nvtcs, vtcs);
        }
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IALLTOALLW_SCHED_INTRA_INPLACE);

    return mpi_errno;
}


/* Non-blocking inplace based ALLTOALLW */
#undef FUNCNAME
#define FUNCNAME MPIR_TSP_Ialltoallw_intra_inplace
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_TSP_Ialltoallw_intra_inplace(const void *sendbuf, const int sendcounts[],
                                      const int sdispls[], const MPI_Datatype sendtypes[],
                                      void *recvbuf, const int recvcounts[],
                                      const int rdispls[], const MPI_Datatype recvtypes[],
                                      MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    int tag;
    MPIR_TSP_sched_t *sched;
    *req = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IALLTOALLW_INTRA_INPLACE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IALLTOALLW_INTRA_INPLACE);


    /* generate the schedule */
    sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
    MPIR_TSP_sched_create(sched);

    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    mpi_errno = MPIR_Sched_next_tag(comm, &tag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno =
        MPIR_TSP_Ialltoallw_sched_intra_inplace(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                                                recvcounts, rdispls, recvtypes, tag, comm, sched);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* start and register the schedule */
    mpi_errno = MPIR_TSP_sched_start(sched, comm, req);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IALLTOALLW_INTRA_INPLACE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
