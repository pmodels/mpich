/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* Header protection (i.e., IALLREDUCE_TSP_RING_ALGOS_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "algo_common.h"
#include "tsp_namespace_def.h"
#include "../iallgatherv/iallgatherv_tsp_ring_algos_prototypes.h"

/* Routine to schedule a ring exchange based allreduce.
 * The implementation is based on Baidu's ring algorithm
 * for Machine Learning/Deep Learning. The algorithm is
 * explained here: http://andrew.gibiansky.com/ */
int MPIR_TSP_Iallreduce_sched_intra_ring(const void *sendbuf, void *recvbuf, int count,
                                         MPI_Datatype datatype, MPI_Op op,
                                         MPIR_Comm * comm, MPIR_TSP_sched_t * sched)
{
    int mpi_errno = MPI_SUCCESS;
    int i, src, dst;
    int nranks, is_inplace, rank;
    size_t extent;
    MPI_Aint lb, true_extent;
    int *cnts, *displs, recv_id, *reduce_id, nvtcs, vtcs;
    int send_rank, recv_rank, total_count;
    void *tmpbuf;
    int tag;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IALLREDUCE_SCHED_INTRA_RING);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IALLREDUCE_SCHED_INTRA_RING);
    MPIR_CHKLMEM_DECL(4);

    is_inplace = (sendbuf == MPI_IN_PLACE);
    nranks = MPIR_Comm_size(comm);
    rank = MPIR_Comm_rank(comm);

    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &lb, &true_extent);
    extent = MPL_MAX(extent, true_extent);

    MPIR_CHKLMEM_MALLOC(cnts, int *, nranks * sizeof(int), mpi_errno, "cnts", MPL_MEM_COLL);
    MPIR_CHKLMEM_MALLOC(displs, int *, nranks * sizeof(int), mpi_errno, "displs", MPL_MEM_COLL);

    for (i = 0; i < nranks; i++)
        cnts[i] = 0;

    total_count = 0;
    for (i = 0; i < nranks; i++) {
        cnts[i] = (count + nranks - 1) / nranks;
        if (total_count + cnts[i] > count) {
            cnts[i] = count - total_count;
            break;
        } else
            total_count += cnts[i];
    }

    displs[0] = 0;
    for (i = 1; i < nranks; i++)
        displs[i] = displs[i - 1] + cnts[i - 1];

    /* Phase 1: copy to tmp buf */
    if (!is_inplace) {
        MPIR_TSP_sched_localcopy(sendbuf, count, datatype, recvbuf, count, datatype, sched, 0,
                                 NULL);
        MPIR_TSP_sched_fence(sched);
    }

    /* Phase 2: Ring based send recv reduce scatter */
    /* Need only 2 spaces for current and previous reduce_id(s) */
    MPIR_CHKLMEM_MALLOC(reduce_id, int *, 2 * sizeof(int), mpi_errno, "reduce_id", MPL_MEM_COLL);
    tmpbuf = MPIR_TSP_sched_malloc(count * extent, sched);

    src = (nranks + rank - 1) % nranks;
    dst = (rank + 1) % nranks;

    for (i = 0; i < nranks - 1; i++) {
        recv_rank = (nranks + rank - 2 - i) % nranks;
        send_rank = (nranks + rank - 1 - i) % nranks;

        /* get a new tag to prevent out of order messages */
        mpi_errno = MPIR_Sched_next_tag(comm, &tag);
        MPIR_ERR_CHECK(mpi_errno);

        nvtcs = (i == 0) ? 0 : 1;
        vtcs = (i == 0) ? 0 : reduce_id[(i - 1) % 2];
        recv_id =
            MPIR_TSP_sched_irecv(tmpbuf, cnts[recv_rank], datatype, src, tag, comm, sched, nvtcs,
                                 &vtcs);

        reduce_id[i % 2] =
            MPIR_TSP_sched_reduce_local(tmpbuf, (char *) recvbuf + displs[recv_rank] * extent,
                                        cnts[recv_rank], datatype, op, sched, 1, &recv_id);

        MPIR_TSP_sched_isend((char *) recvbuf + displs[send_rank] * extent, cnts[send_rank],
                             datatype, dst, tag, comm, sched, nvtcs, &vtcs);

        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                        (MPL_DBG_FDEST,
                         "displs[recv_rank:%d]:%d, cnts[recv_rank:%d, displs[send_rank:%d]:%d, cnts[send_rank:%d]:%d]:%d ",
                         recv_rank, displs[recv_rank], recv_rank, cnts[recv_rank], send_rank,
                         displs[send_rank], send_rank, cnts[send_rank]));
    }
    MPIR_CHKLMEM_MALLOC(reduce_id, int *, 2 * sizeof(int), mpi_errno, "reduce_id", MPL_MEM_COLL);

    MPIR_TSP_sched_fence(sched);

    /* Phase 3: Allgatherv ring, so everyone has the reduced data */
    MPIR_TSP_Iallgatherv_sched_intra_ring(MPI_IN_PLACE, -1, MPI_DATATYPE_NULL, recvbuf, cnts,
                                          displs, datatype, comm, sched);

    MPIR_CHKLMEM_FREEALL();

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IALLREDUCE_SCHED_INTRA_RING);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

/* Non-blocking ring based Allreduce */
int MPIR_TSP_Iallreduce_intra_ring(const void *sendbuf, void *recvbuf, int count,
                                   MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                   MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_TSP_sched_t *sched;
    *req = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IALLREDUCE_INTRA_RING);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IALLREDUCE_INTRA_RING);

    /* generate the schedule */
    sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
    MPIR_ERR_CHKANDJUMP(!sched, mpi_errno, MPI_ERR_OTHER, "**nomem");
    MPIR_TSP_sched_create(sched);

    mpi_errno =
        MPIR_TSP_Iallreduce_sched_intra_ring(sendbuf, recvbuf, count, datatype, op, comm, sched);
    MPIR_ERR_CHECK(mpi_errno);

    /* start and register the schedule */
    mpi_errno = MPIR_TSP_sched_start(sched, comm, req);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IALLREDUCE_INTRA_RING);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
