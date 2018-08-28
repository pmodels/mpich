/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* Header protection (i.e., IALLTOALL_TSP_ALGOS_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

/* The below code is an implementation of alltoall ring algorithm,
 * the dependencies are show below (buf1 and buf2 are temporary buffers
 * used for execution of the ring algorithm) :
 *
               send (buf1)           recv (buf2) --> copy (buf2)
                   |                     |              /
                   |                     |             /
                   v                     V           /
copy (buf1)<--recv (buf1)            send (buf2)   /
      \            |                     |       /
        \          |                     |     /
          \        V                     V   v
            \  send (buf1)           recv (buf2) --> copy (buf2)
             \     |                     |
               \   |                     |
                 v v                     v
               recv (buf1)           send (buf2)
*/
#include "tsp_namespace_def.h"

/* This ring algorithm passes entire buffer around (as if an allgather)
   each round each process picks one portion out of it.

   It does N-1 round N messages with each message N time larger than a
   linear method. However it may prevent congestion in a network topology
   that is missing efficient all-to-all transport.
   -- notes by Hui.
*/

/* Routine to schedule a ring based allgather */
int MPIR_TSP_Ialltoall_sched_intra_ring(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                        void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                        MPIR_Comm * comm, MPIR_TSP_sched_t * sched)
{
    int mpi_errno = MPI_SUCCESS;
    int i, src, dst, copy_dst;

    /* Temporary buffers to execute the ring algorithm */
    void *buf1, *buf2, *data_buf, *sbuf, *rbuf;
    int tag;

    int size = MPIR_Comm_size(comm);
    int rank = MPIR_Comm_rank(comm);
    int is_inplace = (sendbuf == MPI_IN_PLACE);

    MPI_Aint recvtype_lb, recvtype_extent;
    MPI_Aint sendtype_lb, sendtype_extent;
    MPI_Aint sendtype_true_extent, recvtype_true_extent;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IALLTOALL_SCHED_INTRA_RING);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IALLTOALL_SCHED_INTRA_RING);

    /* find out the buffer which has the send data and point data_buf to it */
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

    /* allocate space for temporary buffers */
    buf1 = MPIR_TSP_sched_malloc(size * recvcount * recvtype_extent, sched);
    buf2 = MPIR_TSP_sched_malloc(size * recvcount * recvtype_extent, sched);

    /* copy my data to buf1 which will be forwarded in phase 0 of the ring.
     * TODO: We could avoid this copy but that would make the implementation more
     * complicated */
    int dtcopy_id[3];
    dtcopy_id[0] = MPIR_TSP_sched_localcopy((char *) data_buf, size * recvcount, recvtype,
                                            (char *) buf1, size * recvcount, recvtype, sched, 0,
                                            NULL);

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST,
                     "rank =%d, data_buf=%p, sendbuf=%p, buf1=%p, buf2=%p, recvbuf=%p",
                     rank, data_buf, sendbuf, buf1, buf2, recvbuf));

    if (!is_inplace) {  /* copy my part of my sendbuf to recv_buf */
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                        (MPL_DBG_FDEST,
                         "copying my part from sendbuf address=%p to my recvbuf address=%p",
                         (char *) sendbuf + rank * sendcount * sendtype_extent,
                         (char *) recvbuf + rank * recvcount * recvtype_extent));

        MPIR_TSP_sched_localcopy((char *) sendbuf + rank * sendcount * sendtype_extent,
                                 sendcount, sendtype,
                                 (char *) recvbuf + rank * recvcount * recvtype_extent,
                                 recvcount, recvtype, sched, 0, NULL);
    }

    /* in ring algorithm, source and destination of messages are fixed */
    src = (size + rank - 1) % size;
    dst = (rank + 1) % size;

    sbuf = buf1;
    rbuf = buf2;
    int send_id[3] = { 0 };     /* warning fix: icc: maybe used before set */
    int recv_id[3] = { 0 };     /* warning fix: icc: maybe used before set */
    for (i = 0; i < size - 1; i++) {
        /* For correctness, transport based collectives need to get the
         * tag from the same pool as schedule based collectives */
        mpi_errno = MPIR_Sched_next_tag(comm, &tag);
        MPIR_ERR_CHECK(mpi_errno);

        int vtcs[3], nvtcs;
        /* schedule send */
        if (i == 0) {
            nvtcs = 1;
            vtcs[0] = dtcopy_id[0];
        } else {
            nvtcs = 2;
            vtcs[0] = recv_id[(i - 1) % 3];
            vtcs[1] = send_id[(i - 1) % 3];
        }

        send_id[i % 3] =
            MPIR_TSP_sched_isend((char *) sbuf, size * recvcount, recvtype, dst, tag, comm, sched,
                                 nvtcs, vtcs);

        /* schedule recv */
        if (i == 0)
            nvtcs = 0;
        else if (i == 1) {
            nvtcs = 1;
            vtcs[0] = send_id[(i - 1) % 3];
            vtcs[1] = recv_id[(i - 1) % 3];
        } else {
            nvtcs = 3;
            vtcs[0] = send_id[(i - 1) % 3];
            vtcs[1] = dtcopy_id[(i - 2) % 3];
            vtcs[2] = recv_id[(i - 1) % 3];
        }

        recv_id[i % 3] =
            MPIR_TSP_sched_irecv((char *) rbuf, size * recvcount, recvtype, src, tag, comm, sched,
                                 nvtcs, vtcs);

        /* destination offset of the copy */
        copy_dst = (size + rank - i - 1) % size;
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                        (MPL_DBG_FDEST, "copying from location=%p to location=%p",
                         (char *) rbuf + rank * recvcount * recvtype_extent,
                         (char *) recvbuf + copy_dst * recvcount * recvtype_extent));
        /* schedule data copy */
        dtcopy_id[i % 3] =
            MPIR_TSP_sched_localcopy((char *) rbuf + rank * recvcount * recvtype_extent, recvcount,
                                     recvtype,
                                     (char *) recvbuf + copy_dst * recvcount * recvtype_extent,
                                     recvcount, recvtype, sched, 1, &recv_id[i % 3]);

        /* swap sbuf and rbuf - using data_buf as intermeidate buffer */
        data_buf = sbuf;
        sbuf = rbuf;
        rbuf = data_buf;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IALLTOALL_SCHED_INTRA_RING);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Non-blocking ring based Alltoall */
int MPIR_TSP_Ialltoall_intra_ring(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                  void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                  MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_TSP_sched_t *sched;
    *req = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IALLTOALL_INTRA_RING);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IALLTOALL_INTRA_RING);

    /* Generate the schedule */
    sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
    MPIR_ERR_CHKANDJUMP(!sched, mpi_errno, MPI_ERR_OTHER, "**nomem");

    MPIR_TSP_sched_create(sched, false);

    mpi_errno =
        MPIR_TSP_Ialltoall_sched_intra_ring(sendbuf, sendcount, sendtype, recvbuf,
                                            recvcount, recvtype, comm, sched);
    MPIR_ERR_CHECK(mpi_errno);

    /* Start and register the schedule */
    mpi_errno = MPIR_TSP_sched_start(sched, comm, req);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IALLTOALL_INTRA_RING);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
