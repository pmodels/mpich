/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

int MPIR_Sendrecv_impl(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                       int dest, int sendtag,
                       void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                       int source, int recvtag, MPIR_Comm * comm_ptr, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = NULL;
    MPIR_Request *rreq = NULL;

    /* FIXME - Performance for small messages might be better if MPID_Send() were used here instead of MPID_Isend() */
    /* If source is MPI_PROC_NULL, create a completed request and return. */
    if (unlikely(source == MPI_PROC_NULL)) {
        rreq = MPIR_Request_create_complete(MPIR_REQUEST_KIND__RECV);
        MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
        MPIR_Status_set_procnull(&rreq->status);
    } else {
        mpi_errno =
            MPID_Irecv(recvbuf, recvcount, recvtype, source, recvtag, comm_ptr,
                       MPIR_CONTEXT_INTRA_PT2PT, &rreq);
        if (mpi_errno != MPI_SUCCESS)
            goto fn_fail;
    }

    /* If dest is MPI_PROC_NULL, create a completed request and return. */
    if (unlikely(dest == MPI_PROC_NULL)) {
        sreq = MPIR_Request_create_complete(MPIR_REQUEST_KIND__SEND);
        MPIR_ERR_CHKANDSTMT(sreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    } else {
        mpi_errno =
            MPID_Isend(sendbuf, sendcount, sendtype, dest, sendtag, comm_ptr,
                       MPIR_CONTEXT_INTRA_PT2PT, &sreq);
        if (mpi_errno != MPI_SUCCESS) {
            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno == MPIX_ERR_NOREQ)
                MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**nomem");
            /* FIXME: should we cancel the pending (possibly completed) receive request or wait for it to complete? */
            MPIR_Request_free(rreq);
            goto fn_fail;
            /* --END ERROR HANDLING-- */
        }
    }

    if (!MPIR_Request_is_complete(sreq) || !MPIR_Request_is_complete(rreq)) {
        MPID_Progress_state progress_state;

        MPID_Progress_start(&progress_state);
        while (!MPIR_Request_is_complete(sreq) || !MPIR_Request_is_complete(rreq)) {
            mpi_errno = MPID_Progress_wait(&progress_state);
            if (mpi_errno != MPI_SUCCESS) {
                /* --BEGIN ERROR HANDLING-- */
                MPID_Progress_end(&progress_state);
                goto fn_fail;
                /* --END ERROR HANDLING-- */
            }

            if (unlikely(MPIR_Request_is_anysrc_mismatched(rreq))) {
                /* --BEGIN ERROR HANDLING-- */
                mpi_errno = MPIR_Request_handle_proc_failed(rreq);
                if (!MPIR_Request_is_complete(sreq)) {
                    MPID_Cancel_send(sreq);
                    MPIR_STATUS_SET_CANCEL_BIT(sreq->status, FALSE);
                }
                goto fn_fail;
                /* --END ERROR HANDLING-- */
            }
        }
        MPID_Progress_end(&progress_state);
    }

    mpi_errno = rreq->status.MPI_ERROR;
    MPIR_Request_extract_status(rreq, status);
    MPIR_Request_free(rreq);

    if (mpi_errno == MPI_SUCCESS) {
        mpi_errno = sreq->status.MPI_ERROR;
    }
    MPIR_Request_free(sreq);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Sendrecv_replace_impl(void *buf, MPI_Aint count, MPI_Datatype datatype, int dest,
                               int sendtag, int source, int recvtag, MPIR_Comm * comm_ptr,
                               MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Request *sreq = NULL;
    MPIR_Request *rreq = NULL;
    void *tmpbuf = NULL;
    MPI_Aint tmpbuf_size = 0;
    MPI_Aint actual_pack_bytes = 0;
    MPIR_CHKLMEM_DECL(1);

    MPIR_FUNC_ENTER;

    if (count > 0 && dest != MPI_PROC_NULL) {
        MPIR_Pack_size(count, datatype, &tmpbuf_size);

        MPIR_CHKLMEM_MALLOC_ORJUMP(tmpbuf, void *, tmpbuf_size, mpi_errno,
                                   "temporary send buffer", MPL_MEM_BUFFER);

        mpi_errno =
            MPIR_Typerep_pack(buf, count, datatype, 0, tmpbuf, tmpbuf_size, &actual_pack_bytes,
                              MPIR_TYPEREP_FLAG_NONE);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* If source is MPI_PROC_NULL, create a completed request and return. */
    if (unlikely(source == MPI_PROC_NULL)) {
        rreq = MPIR_Request_create_complete(MPIR_REQUEST_KIND__RECV);
        MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
        MPIR_Status_set_procnull(&rreq->status);
    } else {
        mpi_errno = MPID_Irecv(buf, count, datatype, source, recvtag,
                               comm_ptr, MPIR_CONTEXT_INTRA_PT2PT, &rreq);
        MPIR_ERR_CHECK(mpi_errno);
    }


    /* If dest is MPI_PROC_NULL, create a completed request and return. */
    if (unlikely(dest == MPI_PROC_NULL)) {
        sreq = MPIR_Request_create_complete(MPIR_REQUEST_KIND__SEND);
        MPIR_ERR_CHKANDSTMT(sreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    } else {
        mpi_errno = MPID_Isend(tmpbuf, actual_pack_bytes, MPI_PACKED, dest,
                               sendtag, comm_ptr, MPIR_CONTEXT_INTRA_PT2PT, &sreq);
        if (mpi_errno != MPI_SUCCESS) {
            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno == MPIX_ERR_NOREQ)
                MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**nomem");
            /* FIXME: should we cancel the pending (possibly completed) receive request or wait for it to complete? */
            MPIR_Request_free(rreq);
            goto fn_fail;
            /* --END ERROR HANDLING-- */
        }
    }

    mpi_errno = MPID_Wait(rreq, MPI_STATUS_IGNORE);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPID_Wait(sreq, MPI_STATUS_IGNORE);
    MPIR_ERR_CHECK(mpi_errno);

    if (status != MPI_STATUS_IGNORE) {
        *status = rreq->status;
    }

    if (mpi_errno == MPI_SUCCESS) {
        mpi_errno = rreq->status.MPI_ERROR;

        if (mpi_errno == MPI_SUCCESS) {
            mpi_errno = sreq->status.MPI_ERROR;
        }
    }

    MPIR_Request_free(sreq);
    MPIR_Request_free(rreq);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Isendrecv_impl(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                        int dest, int sendtag,
                        void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                        int source, int recvtag, MPIR_Comm * comm_ptr, MPIR_Request ** p_req)
{
    int mpi_errno = MPI_SUCCESS;

    if (unlikely(dest == MPI_PROC_NULL && source == MPI_PROC_NULL)) {
        MPIR_Request *lw_req = MPIR_Request_create_complete(MPIR_REQUEST_KIND__SEND);
        MPIR_ERR_CHKANDSTMT(lw_req == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
        *p_req = lw_req;
        goto fn_exit;
    } else if (unlikely(source == MPI_PROC_NULL)) {
        /* recv from MPI_PROC_NULL, just send */
        mpi_errno = MPID_Isend(sendbuf, sendcount, sendtype, dest, sendtag, comm_ptr,
                               MPIR_CONTEXT_INTRA_PT2PT, p_req);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    } else if (unlikely(dest == MPI_PROC_NULL)) {
        /* send to MPI_PROC_NULL, just recv */
        mpi_errno = MPID_Irecv(recvbuf, recvcount, recvtype, source, recvtag, comm_ptr,
                               MPIR_CONTEXT_INTRA_PT2PT, p_req);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    MPIR_Sched_t s = MPIR_SCHED_NULL;

    mpi_errno = MPIR_Sched_create(&s, MPIR_SCHED_KIND_REGULAR);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Sched_pt2pt_send(sendbuf, sendcount, sendtype, sendtag, dest, comm_ptr, s);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPIR_Sched_pt2pt_recv(recvbuf, recvcount, recvtype, recvtag, source, comm_ptr, s);
    MPIR_ERR_CHECK(mpi_errno);

    /* note: we are not using collective tag */
    mpi_errno = MPIR_Sched_start(s, comm_ptr, p_req);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int release_temp_buffer(MPIR_Comm * comm_ptr, int tag, void *state)
{
    MPL_free(state);
    return MPI_SUCCESS;
}

int MPIR_Isendrecv_replace_impl(void *buf, MPI_Aint count, MPI_Datatype datatype, int dest,
                                int sendtag, int source, int recvtag, MPIR_Comm * comm_ptr,
                                MPIR_Request ** p_req)
{
    int mpi_errno = MPI_SUCCESS;

    if (unlikely(dest == MPI_PROC_NULL && source == MPI_PROC_NULL)) {
        MPIR_Request *lw_req = MPIR_Request_create_complete(MPIR_REQUEST_KIND__SEND);
        MPIR_ERR_CHKANDSTMT(lw_req == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
        *p_req = lw_req;
        goto fn_exit;
    } else if (unlikely(source == MPI_PROC_NULL)) {
        /* recv from MPI_PROC_NULL, just send */
        mpi_errno = MPID_Isend(buf, count, datatype, dest, sendtag, comm_ptr,
                               MPIR_CONTEXT_INTRA_PT2PT, p_req);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    } else if (unlikely(dest == MPI_PROC_NULL)) {
        /* send to MPI_PROC_NULL, just recv */
        mpi_errno = MPID_Irecv(buf, count, datatype, source, recvtag, comm_ptr,
                               MPIR_CONTEXT_INTRA_PT2PT, p_req);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    void *tmpbuf = NULL;
    if (count > 0 && dest != MPI_PROC_NULL) {
        MPI_Aint tmpbuf_size = 0;
        MPI_Aint actual_pack_bytes;
        MPIR_Pack_size(count, datatype, &tmpbuf_size);

        tmpbuf = MPL_malloc(tmpbuf_size, MPL_MEM_BUFFER);
        if (!tmpbuf) {
            MPIR_CHKMEM_SETERR(mpi_errno, tmpbuf_size, "temporary send buffer");
            goto fn_fail;
        }

        mpi_errno = MPIR_Typerep_pack(buf, count, datatype, 0, tmpbuf, tmpbuf_size,
                                      &actual_pack_bytes, MPIR_TYPEREP_FLAG_NONE);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Assert(tmpbuf_size == actual_pack_bytes);
    }

    MPIR_Sched_t s = MPIR_SCHED_NULL;

    mpi_errno = MPIR_Sched_create(&s, MPIR_SCHED_KIND_REGULAR);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Sched_pt2pt_send(tmpbuf, count, datatype, sendtag, dest, comm_ptr, s);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPIR_Sched_pt2pt_recv(buf, count, datatype, recvtag, source, comm_ptr, s);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Sched_barrier(s);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPIR_Sched_cb(&release_temp_buffer, tmpbuf, s);
    MPIR_ERR_CHECK(mpi_errno);

    /* note: we are not using collective tag */
    mpi_errno = MPIR_Sched_start(s, comm_ptr, p_req);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
