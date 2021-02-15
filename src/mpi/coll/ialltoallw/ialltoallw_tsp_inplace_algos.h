/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* Header protection (i.e., IALLTOALLW_TSP_ALGOS_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "algo_common.h"
#include "tsp_namespace_def.h"

/* Routine to schedule inplace algorithm for  alltoallw */
int MPIR_TSP_Ialltoallw_sched_intra_inplace(const void *sendbuf, const int sendcounts[],
                                            const int sdispls[], const MPI_Datatype sendtypes[],
                                            void *recvbuf, const int recvcounts[],
                                            const int rdispls[], const MPI_Datatype recvtypes[],
                                            MPIR_Comm * comm, MPIR_TSP_sched_t * sched)
{
    int mpi_errno = MPI_SUCCESS;
    int tag;
    size_t recv_extent;
    MPI_Aint true_extent, true_lb;
    int nranks, rank, nvtcs;
    int i, dst, send_id, recv_id, dtcopy_id = -1, vtcs[2];
    int max_size;
    void *tmp_buf = NULL, *adj_tmp_buf = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IALLTOALLW_SCHED_INTRA_INPLACE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IALLTOALLW_SCHED_INTRA_INPLACE);

    MPIR_Assert(sendbuf == MPI_IN_PLACE);

    nranks = MPIR_Comm_size(comm);
    rank = MPIR_Comm_rank(comm);

    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    mpi_errno = MPIR_Sched_next_tag(comm, &tag);
    MPIR_ERR_CHECK(mpi_errno);

    max_size = 0;
    for (i = 0; i < nranks; ++i) {
        /* only look at recvtypes/recvcounts because the send vectors are
         * ignored when sendbuf==MPI_IN_PLACE */
        MPIR_Type_get_true_extent_impl(recvtypes[i], &true_lb, &true_extent);
        MPIR_Datatype_get_extent_macro(recvtypes[i], recv_extent);
        max_size = MPL_MAX(max_size, recvcounts[i] * MPL_MAX(recv_extent, true_extent));
    }

    tmp_buf = MPIR_TSP_sched_malloc(max_size, sched);
    MPIR_ERR_CHKANDJUMP(!tmp_buf, mpi_errno, MPI_ERR_OTHER, "**nomem");

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

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IALLTOALLW_SCHED_INTRA_INPLACE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


/* Non-blocking inplace based ALLTOALLW */
int MPIR_TSP_Ialltoallw_intra_inplace(const void *sendbuf, const int sendcounts[],
                                      const int sdispls[], const MPI_Datatype sendtypes[],
                                      void *recvbuf, const int recvcounts[],
                                      const int rdispls[], const MPI_Datatype recvtypes[],
                                      MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_TSP_sched_t *sched;
    *req = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IALLTOALLW_INTRA_INPLACE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IALLTOALLW_INTRA_INPLACE);


    /* generate the schedule */
    sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
    MPIR_ERR_CHKANDJUMP(!sched, mpi_errno, MPI_ERR_OTHER, "**nomem");
    MPIR_TSP_sched_create(sched, false);

    mpi_errno =
        MPIR_TSP_Ialltoallw_sched_intra_inplace(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                                                recvcounts, rdispls, recvtypes, comm, sched);
    MPIR_ERR_CHECK(mpi_errno);

    /* start and register the schedule */
    mpi_errno = MPIR_TSP_sched_start(sched, comm, req);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IALLTOALLW_INTRA_INPLACE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
