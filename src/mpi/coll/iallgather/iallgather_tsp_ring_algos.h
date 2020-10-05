/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* Header protection (i.e., IALLGATHER_TSP_RING_ALGOS_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "tsp_namespace_def.h"

/* Routine to schedule a ring based allgather */
int MPIR_TSP_Iallgather_sched_intra_ring(const void *sendbuf, int sendcount,
                                         MPI_Datatype sendtype, void *recvbuf,
                                         int recvcount, MPI_Datatype recvtype,
                                         MPIR_Comm * comm, MPIR_TSP_sched_t * sched)
{
    int mpi_errno = MPI_SUCCESS;
    int i, src, dst, copy_dst;
    /* Temporary buffers to execute the ring algorithm */
    void *buf1, *buf2, *data_buf, *rbuf, *sbuf;

    int size = MPIR_Comm_size(comm);
    int rank = MPIR_Comm_rank(comm);
    int is_inplace = (sendbuf == MPI_IN_PLACE);
    int tag;

    MPI_Aint recvtype_lb, recvtype_extent;
    MPI_Aint sendtype_lb, sendtype_extent;
    MPI_Aint sendtype_true_extent, recvtype_true_extent;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IALLGATHER_SCHED_INTRA_RING);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IALLGATHER_SCHED_INTRA_RING);

    /* Find out the buffer which has the send data and point data_buf to it */
    if (is_inplace) {
        sendcount = recvcount;
        sendtype = recvtype;
        data_buf = (char *) recvbuf;
    } else
        data_buf = (char *) sendbuf;

    /* Get datatype info of sendtype and recvtype */
    MPIR_Datatype_get_extent_macro(sendtype, sendtype_extent);
    MPIR_Type_get_true_extent_impl(sendtype, &sendtype_lb, &sendtype_true_extent);
    sendtype_extent = MPL_MAX(sendtype_extent, sendtype_true_extent);

    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);
    MPIR_Type_get_true_extent_impl(recvtype, &recvtype_lb, &recvtype_true_extent);
    recvtype_extent = MPL_MAX(recvtype_extent, recvtype_true_extent);

    /* Allocate space for temporary buffers */
    buf1 = MPIR_TSP_sched_malloc(recvcount * recvtype_extent, sched);
    buf2 = MPIR_TSP_sched_malloc(recvcount * recvtype_extent, sched);

    int dtcopy_id[3];
    if (is_inplace) {
        /* Copy data to buf1 from sendbuf or recvbuf(in case of inplace) */
        dtcopy_id[0] =
            MPIR_TSP_sched_localcopy((char *) data_buf + rank * recvcount * recvtype_extent,
                                     sendcount, sendtype, (char *) buf1, recvcount, recvtype, sched,
                                     0, NULL);
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                        (MPL_DBG_FDEST,
                         "copying data  to tmp_buf  on add:%p on %d rank before for loop from sendbuf:%p and dtcopy_id:%d",
                         buf1, rank, (char *) data_buf + rank * sendtype_extent, dtcopy_id[0]));
    } else {
        /* Copy your data into your recvbuf from your sendbuf */
        MPIR_TSP_sched_localcopy((char *) sendbuf, sendcount, sendtype,
                                 (char *) recvbuf + rank * recvcount * recvtype_extent,
                                 recvcount, recvtype, sched, 0, NULL);

        /* Copy data from sendbuf to buf1 to send the data */
        dtcopy_id[0] = MPIR_TSP_sched_localcopy((char *) data_buf, sendcount, sendtype,
                                                (char *) buf1, recvcount, recvtype, sched, 0, NULL);
    }

    /* In ring algorithm src and dst are fixed */
    src = (size + rank - 1) % size;
    dst = (rank + 1) % size;

    sbuf = buf1;
    rbuf = buf2;

    /* Ranks pass around the data (size - 1) times */
    int send_id[3];
    int recv_id[3] = { 0 };     /* warning fix: icc: maybe used before set */
    for (i = 0; i < size - 1; i++) {
        /* Get new tag for each cycle so that the send-recv pairs are matched correctly */
        mpi_errno = MPIR_Sched_next_tag(comm, &tag);
        MPIR_ERR_CHECK(mpi_errno);

        int vtcs[3], nvtcs;
        if (i == 0) {
            nvtcs = 1;
            vtcs[0] = dtcopy_id[0];
            send_id[0] = MPIR_TSP_sched_isend((char *) sbuf, recvcount, recvtype,
                                              dst, tag, comm, sched, nvtcs, vtcs);
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST, "posting recv at address=%p, count=%d", rbuf,
                             size * recvcount));
            nvtcs = 0;
        } else {
            nvtcs = 2;
            vtcs[0] = recv_id[(i - 1) % 3];
            vtcs[1] = send_id[(i - 1) % 3];
            send_id[i % 3] = MPIR_TSP_sched_isend((char *) sbuf, recvcount, recvtype,
                                                  dst, tag, comm, sched, nvtcs, vtcs);
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST, "posting recv at address=%p, count=%d", rbuf,
                             size * recvcount));
            if (i == 1) {
                nvtcs = 2;
                vtcs[0] = send_id[0];
                vtcs[1] = recv_id[0];
            } else {
                nvtcs = 3;
                vtcs[0] = send_id[(i - 1) % 3];
                vtcs[1] = dtcopy_id[(i - 2) % 3];
                vtcs[2] = recv_id[(i - 1) % 3];
            }
        }

        recv_id[i % 3] = MPIR_TSP_sched_irecv((char *) rbuf, recvcount, recvtype,
                                              src, tag, comm, sched, nvtcs, vtcs);

        copy_dst = (size + rank - i - 1) % size;        /* Destination offset of the copy */
        dtcopy_id[i % 3] = MPIR_TSP_sched_localcopy((char *) rbuf, recvcount, recvtype,
                                                    (char *) recvbuf +
                                                    copy_dst * recvcount * recvtype_extent,
                                                    recvcount, recvtype, sched, 1, &recv_id[i % 3]);
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                        (MPL_DBG_FDEST, "copying from location=%p to location=%p",
                         (char *) rbuf + rank * recvcount * recvtype_extent,
                         (char *) recvbuf + copy_dst * recvcount * recvtype_extent));

        data_buf = sbuf;
        sbuf = rbuf;
        rbuf = data_buf;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IALLGATHER_SCHED_INTRA_RING);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

/* Non-blocking ring based Allgather */
int MPIR_TSP_Iallgather_intra_ring(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                   void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                   MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_TSP_sched_t *sched;
    *req = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IALLGATHER_INTRA_RING);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IALLGATHER_INTRA_RING);


    /* Generate the schedule */
    sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
    MPIR_ERR_CHKANDJUMP(!sched, mpi_errno, MPI_ERR_OTHER, "**nomem");
    MPIR_TSP_sched_create(sched);

    mpi_errno =
        MPIR_TSP_Iallgather_sched_intra_ring(sendbuf, sendcount, sendtype, recvbuf,
                                             recvcount, recvtype, comm, sched);
    MPIR_ERR_CHECK(mpi_errno);

    /* Start and register the schedule */
    mpi_errno = MPIR_TSP_sched_start(sched, comm, req);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IALLGATHER_INTRA_RING);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
