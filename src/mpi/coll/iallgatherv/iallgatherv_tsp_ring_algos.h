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

/* Header protection (i.e., IALLGATHERV_TSP_RING_ALGOS_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "algo_common.h"
#include "tsp_namespace_def.h"

/* Routine to schedule a recursive exchange based allgatherv */
#undef FUNCNAME
#define FUNCNAME MPIR_TSP_Iallgatherv_sched_intra_ring
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_TSP_Iallgatherv_sched_intra_ring(const void *sendbuf, int sendcount,
                                          MPI_Datatype sendtype, void *recvbuf,
                                          const int *recvcounts, const int *displs,
                                          MPI_Datatype recvtype, int tag, MPIR_Comm * comm,
                                          MPIR_TSP_sched_t * sched)
{
    size_t extent;
    MPI_Aint lb, true_extent;
    int mpi_errno = MPI_SUCCESS;
    int i, src, dst;
    int nranks, is_inplace, rank;
    int nvtcs, vtcs[3], send_id[3], recv_id[3], dtcopy_id[3];
    int send_rank, recv_rank;
    void *data_buf, *buf1, *buf2, *sbuf, *rbuf;
    int max_count;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IALLGATHERV_SCHED_INTRA_RING);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IALLGATHERV_SCHED_INTRA_RING);

    is_inplace = (sendbuf == MPI_IN_PLACE);
    nranks = MPIR_Comm_size(comm);
    rank = MPIR_Comm_rank(comm);

    /* find out the buffer which has the send data and point data_buf to it */
    if (is_inplace) {
        sendcount = recvcounts[rank];
        sendtype = recvtype;
        data_buf = (char *) recvbuf;
    } else
        data_buf = (char *) sendbuf;

    MPIR_Datatype_get_extent_macro(recvtype, extent);
    MPIR_Type_get_true_extent_impl(recvtype, &lb, &true_extent);
    extent = MPL_MAX(extent, true_extent);

    max_count = recvcounts[0];
    for (i = 1; i < nranks; i++) {
        if (recvcounts[i] > max_count)
            max_count = recvcounts[i];
    }

    /* allocate space for temporary buffers to accommodate the largest recvcount */
    buf1 = MPIR_TSP_sched_malloc(max_count * extent, sched);
    buf2 = MPIR_TSP_sched_malloc(max_count * extent, sched);

    /* Phase 1: copy data to buf1 from sendbuf or recvbuf(in case of inplace) */
    if (is_inplace) {
        dtcopy_id[0] =
            MPIR_TSP_sched_localcopy((char *) data_buf + displs[rank] * extent, sendcount, sendtype,
                                     buf1, recvcounts[rank], recvtype, sched, 0, NULL);
    } else {
        /* copy your data into your recvbuf from your sendbuf */
        MPIR_TSP_sched_localcopy(sendbuf, sendcount, sendtype,
                                 (char *) recvbuf + displs[rank] * extent, recvcounts[rank],
                                 recvtype, sched, 0, NULL);
        /* copy data from sendbuf to tmp_sendbuf to send the data */
        dtcopy_id[0] =
            MPIR_TSP_sched_localcopy(sendbuf, sendcount, sendtype, buf1, recvcounts[rank], recvtype,
                                     sched, 0, NULL);
    }

    src = (nranks + rank - 1) % nranks;
    dst = (rank + 1) % nranks;

    sbuf = buf1;
    rbuf = buf2;

    for (i = 0; i < nranks - 1; i++) {
        recv_rank = (rank - i - 1 + nranks) % nranks;   /* Rank whose data you're receiving */
        send_rank = (rank - i + nranks) % nranks;       /* Rank whose data you're sending */

        if (i == 0) {
            nvtcs = 1;
            vtcs[0] = dtcopy_id[0];
        } else {
            nvtcs = 2;
            vtcs[0] = recv_id[(i - 1) % 3];
            vtcs[1] = send_id[(i - 1) % 3];
        }

        send_id[i % 3] =
            MPIR_TSP_sched_isend(sbuf, recvcounts[send_rank], recvtype, dst, tag, comm, sched,
                                 nvtcs, vtcs);


        if (i == 0) {
            nvtcs = 0;
        } else if (i == 1) {
            nvtcs = 2;
            vtcs[0] = send_id[(i - 1) % 3];
            vtcs[1] = recv_id[(i - 1) % 3];
        } else {
            nvtcs = 3;
            vtcs[0] = send_id[(i - 1) % 3];
            vtcs[1] = dtcopy_id[(i - 2) % 3];
            vtcs[2] = recv_id[(i - 1) % 3];
        }

        recv_id[i % 3] =
            MPIR_TSP_sched_irecv(rbuf, recvcounts[recv_rank], recvtype, src, tag, comm, sched,
                                 nvtcs, vtcs);

        /* Copy to correct position in recvbuf */
        dtcopy_id[i % 3] =
            MPIR_TSP_sched_localcopy(rbuf, recvcounts[recv_rank], recvtype,
                                     (char *) recvbuf + displs[recv_rank] * extent,
                                     recvcounts[recv_rank], recvtype, sched, 1, &recv_id[i % 3]);

        data_buf = sbuf;
        sbuf = rbuf;
        rbuf = data_buf;

    }

    MPIR_TSP_sched_fence(sched);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IALLGATHERV_SCHED_INTRA_RING);
    return mpi_errno;
}


/* Non-blocking ring based Allgatherv */
#undef FUNCNAME
#define FUNCNAME MPIR_TSP_Iallgatherv_intra_ring
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_TSP_Iallgatherv_intra_ring(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                    void *recvbuf, const int *recvcounts, const int *displs,
                                    MPI_Datatype recvtype, MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    int tag;
    MPIR_TSP_sched_t *sched;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IALLGATHERV_INTRA_RING);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IALLGATHERV_INTRA_RING);

    *req = NULL;

    /* generate the schedule */
    sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
    MPIR_Assert(sched != NULL);
    MPIR_TSP_sched_create(sched);

    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    mpi_errno = MPIDU_Sched_next_tag(comm, &tag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno =
        MPIR_TSP_Iallgatherv_sched_intra_ring(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                              displs, recvtype, tag, comm, sched);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* start and register the schedule */
    mpi_errno = MPIR_TSP_sched_start(sched, comm, req);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IALLGATHERV_INTRA_RING);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
