/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

#ifdef ENABLE_THREADCOMM

#include "threadcomm_pt2pt.h"

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
        /* TODO */
        MPIR_Assert(0);
        /* use threadcomm->comm, but we need patch the request or tag for proper matching */
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

    if (MPIR_THREADCOMM_RANK_IS_INTERTHREAD(threadcomm, rank)) {
        int src_id = MPIR_THREADCOMM_RANK_TO_TID(threadcomm, rank);
        mpi_errno = MPIR_Threadcomm_recv(buf, count, datatype, src_id, tag, threadcomm, attr,
                                         req, true);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        /* TODO */
        MPIR_Assert(0);
        /* use threadcomm->comm, but we need patch the request or tag for proper matching */
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
        /* TODO */
        MPIR_Assert(0);
        /* use threadcomm->comm, but we need patch the request or tag for proper matching */
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
        /* TODO */
        MPIR_Assert(0);
        /* use threadcomm->comm, but we need patch the request or tag for proper matching */
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
