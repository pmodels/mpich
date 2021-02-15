/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* Header protection (i.e., IALLTOALLV_TSP_ALGOS_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "algo_common.h"
#include "tsp_namespace_def.h"

/* Routine to schedule a recursive exchange based alltoallv */
int MPIR_TSP_Ialltoallv_sched_intra_inplace(const void *sendbuf, const int sendcounts[],
                                            const int sdispls[], MPI_Datatype sendtype,
                                            void *recvbuf, const int recvcounts[],
                                            const int rdispls[], MPI_Datatype recvtype,
                                            MPIR_Comm * comm, MPIR_TSP_sched_t * sched)
{
    int mpi_errno = MPI_SUCCESS;
    size_t recv_extent;
    MPI_Aint recv_lb, true_extent;
    int nranks, rank, nvtcs;
    int i, dst, send_id, recv_id, dtcopy_id = -1, vtcs[2];
    int max_count;
    void *tmp_buf = NULL;
    int tag = 0;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IALLTOALLV_SCHED_INTRA_INPLACE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IALLTOALLV_SCHED_INTRA_INPLACE);

    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    mpi_errno = MPIR_Sched_next_tag(comm, &tag);
    MPIR_ERR_CHECK(mpi_errno);

    nranks = MPIR_Comm_size(comm);
    rank = MPIR_Comm_rank(comm);

    MPIR_Datatype_get_extent_macro(recvtype, recv_extent);
    MPIR_Type_get_true_extent_impl(recvtype, &recv_lb, &true_extent);
    recv_extent = MPL_MAX(recv_extent, true_extent);

    max_count = 0;
    for (i = 0; i < nranks; ++i) {
        max_count = MPL_MAX(max_count, recvcounts[i]);
    }

    tmp_buf = MPIR_TSP_sched_malloc(max_count * recv_extent, sched);

    for (i = 0; i < nranks; ++i) {
        if (rank != i) {
            dst = i;
            nvtcs = (dtcopy_id == -1) ? 0 : 1;
            vtcs[0] = dtcopy_id;

            send_id = MPIR_TSP_sched_isend((char *) recvbuf + rdispls[dst] * recv_extent,
                                           recvcounts[dst], recvtype, dst, tag, comm,
                                           sched, nvtcs, vtcs);

            recv_id =
                MPIR_TSP_sched_irecv(tmp_buf, recvcounts[dst], recvtype, dst, tag, comm,
                                     sched, nvtcs, vtcs);

            nvtcs = 2;
            vtcs[0] = send_id;
            vtcs[1] = recv_id;
            dtcopy_id = MPIR_TSP_sched_localcopy(tmp_buf, recvcounts[dst], recvtype,
                                                 ((char *) recvbuf +
                                                  rdispls[dst] * recv_extent), recvcounts[dst],
                                                 recvtype, sched, nvtcs, vtcs);
        }
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IALLTOALLV_SCHED_INTRA_INPLACE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


/* Non-blocking inplace based ALLTOALLV */
int MPIR_TSP_Ialltoallv_intra_inplace(const void *sendbuf, const int sendcounts[],
                                      const int sdispls[], MPI_Datatype sendtype, void *recvbuf,
                                      const int recvcounts[], const int rdispls[],
                                      MPI_Datatype recvtype, MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_TSP_sched_t *sched;
    *req = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IALLTOALLV_INTRA_INPLACE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IALLTOALLV_INTRA_INPLACE);


    /* generate the schedule */
    sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
    MPIR_Assert(sched != NULL);
    MPIR_TSP_sched_create(sched, false);

    mpi_errno =
        MPIR_TSP_Ialltoallv_sched_intra_inplace(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                                                recvcounts, rdispls, recvtype, comm, sched);
    MPIR_ERR_CHECK(mpi_errno);

    /* start and register the schedule */
    mpi_errno = MPIR_TSP_sched_start(sched, comm, req);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IALLTOALLV_INTRA_INPLACE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
