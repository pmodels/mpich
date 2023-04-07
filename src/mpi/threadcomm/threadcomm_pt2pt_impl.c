/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

#ifdef ENABLE_THREADCOMM

#include "threadcomm_pt2pt.h"

/* NOTE: anysource and anytag is not supported in threadcomm. It is too complicated
 *       to manage matching cross inter-thread, intra-node, and inter-node.
 */

#define THREADCOMM_INTERPROCESS_TAG(tag, src_id, dst_id) \
    (((((src_id) << MPIR_TAG_THREADCOMM_TID_BITS) + (dst_id)) << MPIR_TAG_THREADCOMM_USABLE_BITS) + (tag))

#define THREADCOMM_INTERPROCESS_GET_RANK_ID(threadcomm, rank, local_id, remote_rank, remote_id) \
    do { \
        local_id = MPIR_threadcomm_get_tid(threadcomm); \
        int comm_size = threadcomm->comm->local_size; \
        remote_rank = -1; \
        remote_id = -1; \
        for (int i = 0; i < comm_size; i++) { \
            if (rank < threadcomm->rank_offset_table[i]) { \
                remote_rank = i; \
                remote_id = (i == 0 ? rank : rank - threadcomm->rank_offset_table[i - 1]); \
                break; \
            } \
        } \
    } while (0)

int MPIR_Threadcomm_isend_attr(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                               int rank, int tag, MPIR_Threadcomm * threadcomm, int attr,
                               MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIR_THREADCOMM_RANK_IS_INTERTHREAD(threadcomm, rank)) {
        int dst_id = MPIR_THREADCOMM_RANK_TO_TID(threadcomm, rank);
        mpi_errno = MPIR_Threadcomm_send(buf, count, datatype, dst_id, tag, threadcomm, attr, req);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        int src_id, dst_rank, dst_id, new_tag;
        THREADCOMM_INTERPROCESS_GET_RANK_ID(threadcomm, rank, src_id, dst_rank, dst_id);
        new_tag = THREADCOMM_INTERPROCESS_TAG(tag, src_id, dst_id);
        mpi_errno =
            MPID_Isend(buf, count, datatype, dst_rank, new_tag, threadcomm->comm, attr, req);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Threadcomm_irecv_attr(void *buf, MPI_Aint count, MPI_Datatype datatype,
                               int rank, int tag, MPIR_Threadcomm * threadcomm, int attr,
                               MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    if (rank == MPI_ANY_SOURCE) {
        if (threadcomm->comm->local_size == 1) {
            mpi_errno = MPIR_Threadcomm_recv(buf, count, datatype,
                                             MPI_ANY_SOURCE, tag, threadcomm, attr, req, true);
            MPIR_ERR_CHECK(mpi_errno);
        } else {
            /* FIXME */
            MPIR_Assert(0 && "MPI_ANY_SOURCE on interprocess threadcomm not supported yet");
        }
    } else if (MPIR_THREADCOMM_RANK_IS_INTERTHREAD(threadcomm, rank)) {
        int src_id = MPIR_THREADCOMM_RANK_TO_TID(threadcomm, rank);
        mpi_errno = MPIR_Threadcomm_recv(buf, count, datatype, src_id, tag, threadcomm, attr,
                                         req, true);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        int dst_id, src_rank, src_id, new_tag;
        THREADCOMM_INTERPROCESS_GET_RANK_ID(threadcomm, rank, dst_id, src_rank, src_id);
        new_tag = THREADCOMM_INTERPROCESS_TAG(tag, src_id, dst_id);
        mpi_errno =
            MPID_Irecv(buf, count, datatype, src_rank, new_tag, threadcomm->comm, attr, req);
        MPIR_ERR_CHECK(mpi_errno);
        /* TODO: fix status.MPI_TAG in MPI_Wait */
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Threadcomm_isend_impl(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                               int rank, int tag, MPIR_Comm * comm, MPIR_Request ** req)
{
    MPIR_Assert(comm->threadcomm);
    return MPIR_Threadcomm_isend_attr(buf, count, datatype, rank, tag, comm->threadcomm, 0, req);
}

int MPIR_Threadcomm_irecv_impl(void *buf, MPI_Aint count, MPI_Datatype datatype,
                               int rank, int tag, MPIR_Comm * comm, MPIR_Request ** req)
{
    MPIR_Assert(comm->threadcomm);
    return MPIR_Threadcomm_irecv_attr(buf, count, datatype, rank, tag, comm->threadcomm, 0, req);
}

int MPIR_Threadcomm_send_impl(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                              int rank, int tag, MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(comm->threadcomm);
    MPIR_Threadcomm *threadcomm = comm->threadcomm;

    if (MPIR_THREADCOMM_RANK_IS_INTERTHREAD(threadcomm, rank)) {
        MPIR_Request *sreq;
        int dst_id = MPIR_THREADCOMM_RANK_TO_TID(threadcomm, rank);
        mpi_errno = MPIR_Threadcomm_send(buf, count, datatype, dst_id, tag, threadcomm, 0, &sreq);
        MPIR_ERR_CHECK(mpi_errno);
        /* wait */
        while (!MPIR_Request_is_complete(sreq)) {
            MPIR_Threadcomm_progress();
        }
        MPIR_Request_free(sreq);
    } else {
        int src_id, dst_rank, dst_id, new_tag;
        THREADCOMM_INTERPROCESS_GET_RANK_ID(threadcomm, rank, src_id, dst_rank, dst_id);
        new_tag = THREADCOMM_INTERPROCESS_TAG(tag, src_id, dst_id);

        MPIR_Request *sreq;
        mpi_errno = MPID_Send(buf, count, datatype, dst_rank, new_tag, threadcomm->comm, 0, &sreq);
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno = MPID_Wait(sreq, MPI_STATUS_IGNORE);
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno = sreq->status.MPI_ERROR;
        MPIR_Request_free(sreq);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Threadcomm_recv_impl(void *buf, MPI_Aint count, MPI_Datatype datatype,
                              int rank, int tag, MPIR_Comm * comm, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(comm->threadcomm);
    MPIR_Threadcomm *threadcomm = comm->threadcomm;

    if (MPIR_THREADCOMM_RANK_IS_INTERTHREAD(threadcomm, rank)) {
        MPIR_Request *rreq;
        int src_id = MPIR_THREADCOMM_RANK_TO_TID(threadcomm, rank);
        bool has_status = (status != MPI_STATUS_IGNORE);
        mpi_errno = MPIR_Threadcomm_recv(buf, count, datatype, src_id, tag, threadcomm, 0, &rreq,
                                         has_status);
        MPIR_ERR_CHECK(mpi_errno);
        while (!MPIR_Request_is_complete(rreq)) {
            MPIR_Threadcomm_progress();
        }
        mpi_errno = rreq->status.MPI_ERROR;
        MPIR_Request_extract_status(rreq, status);
        MPIR_Request_free(rreq);
    } else {
        int dst_id, src_rank, src_id, new_tag;
        THREADCOMM_INTERPROCESS_GET_RANK_ID(threadcomm, rank, dst_id, src_rank, src_id);
        new_tag = THREADCOMM_INTERPROCESS_TAG(tag, src_id, dst_id);

        MPIR_Request *rreq;
        mpi_errno = MPID_Recv(buf, count, datatype, src_rank, new_tag, threadcomm->comm, 0,
                              status, &rreq);
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno = MPID_Wait(rreq, MPI_STATUS_IGNORE);
        MPIR_ERR_CHECK(mpi_errno);

        rreq->status.MPI_TAG &= (1 << MPIR_TAG_THREADCOMM_USABLE_BITS) - 1;

        mpi_errno = rreq->status.MPI_ERROR;
        MPIR_Request_extract_status(rreq, status);
        MPIR_Request_free(rreq);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Threadcomm_progress(void)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIR_threadcomm_array && utarray_len(MPIR_threadcomm_array)) {
        threadcomm_progress_send();
        threadcomm_progress_recv();
    }

    return mpi_errno;
}

#endif
