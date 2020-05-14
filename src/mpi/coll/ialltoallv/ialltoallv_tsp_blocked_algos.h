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
int MPIR_TSP_Ialltoallv_sched_intra_blocked(const void *sendbuf, const int sendcounts[],
                                            const int sdispls[], MPI_Datatype sendtype,
                                            void *recvbuf, const int recvcounts[],
                                            const int rdispls[], MPI_Datatype recvtype,
                                            MPIR_Comm * comm, int bblock, MPIR_TSP_sched_t * sched)
{
    int mpi_errno = MPI_SUCCESS;
    int is_inplace;
    size_t recv_extent, send_extent, sendtype_size, recvtype_size;
    MPI_Aint recv_lb, send_lb, true_extent;
    int nranks, rank;
    int i, j, comm_block, dst;
    int tag = 0;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IALLTOALLV_SCHED_INTRA_BLOCKED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IALLTOALLV_SCHED_INTRA_BLOCKED);

    is_inplace = (sendbuf == MPI_IN_PLACE);
    MPIR_Assert(!is_inplace);
    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    mpi_errno = MPIR_Sched_next_tag(comm, &tag);
    MPIR_ERR_CHECK(mpi_errno);

    nranks = MPIR_Comm_size(comm);
    rank = MPIR_Comm_rank(comm);

    MPIR_Datatype_get_extent_macro(recvtype, recv_extent);
    MPIR_Type_get_true_extent_impl(recvtype, &recv_lb, &true_extent);
    recv_extent = MPL_MAX(recv_extent, true_extent);
    MPIR_Datatype_get_size_macro(recvtype, recvtype_size);

    MPIR_Datatype_get_extent_macro(sendtype, send_extent);
    MPIR_Type_get_true_extent_impl(sendtype, &send_lb, &true_extent);
    send_extent = MPL_MAX(send_extent, true_extent);
    MPIR_Datatype_get_size_macro(sendtype, sendtype_size);

    if (bblock == 0)
        bblock = nranks;

    /* post only bblock isends/irecvs at a time as suggested by Tony Ladd */
    for (i = 0; i < nranks; i += bblock) {
        comm_block = nranks - i < bblock ? nranks - i : bblock;

        /* do the communication -- post ss sends and receives: */
        for (j = 0; j < comm_block; j++) {
            dst = (rank + j + i) % nranks;
            if (recvcounts[dst] && recvtype_size) {
                MPIR_TSP_sched_irecv((char *) recvbuf + rdispls[dst] * recv_extent,
                                     recvcounts[dst], recvtype, dst, tag, comm, sched, 0, NULL);
            }
        }

        for (j = 0; j < comm_block; j++) {
            dst = (rank - j - i + nranks) % nranks;
            if (sendcounts[dst] && sendtype_size) {
                MPIR_TSP_sched_isend((char *) sendbuf + sdispls[dst] * send_extent,
                                     sendcounts[dst], sendtype, dst, tag, comm, sched, 0, NULL);
            }
        }

        /* force our block of sends/recvs to complete before starting the next block */
        MPIR_TSP_sched_fence((MPIR_TSP_sched_t *) sched);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IALLTOALLV_SCHED_INTRA_BLOCKED);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


/* Non-blocking blocked based ALLTOALLV */
int MPIR_TSP_Ialltoallv_intra_blocked(const void *sendbuf, const int sendcounts[],
                                      const int sdispls[], MPI_Datatype sendtype, void *recvbuf,
                                      const int recvcounts[], const int rdispls[],
                                      MPI_Datatype recvtype, MPIR_Comm * comm, int bblock,
                                      MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_TSP_sched_t *sched;
    *req = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IALLTOALLV_INTRA_BLOCKED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IALLTOALLV_INTRA_BLOCKED);


    /* generate the schedule */
    sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
    MPIR_Assert(sched != NULL);
    MPIR_TSP_sched_create(sched);

    mpi_errno =
        MPIR_TSP_Ialltoallv_sched_intra_blocked(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                                                recvcounts, rdispls, recvtype, comm, bblock, sched);
    MPIR_ERR_CHECK(mpi_errno);

    /* start and register the schedule */
    mpi_errno = MPIR_TSP_sched_start(sched, comm, req);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IALLTOALLV_INTRA_BLOCKED);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
