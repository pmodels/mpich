/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CH4_RECV_H_INCLUDED
#define CH4_RECV_H_INCLUDED

#include "ch4_impl.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_recv_unsafe(void *buf,
                                               MPI_Aint count,
                                               MPI_Datatype datatype,
                                               int rank,
                                               int tag,
                                               MPIR_Comm * comm,
                                               int context_offset, MPIDI_av_entry_t * av,
                                               MPI_Status * status, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_RECV_UNSAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_RECV_UNSAFE);

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno =
        MPIDI_NM_mpi_recv(buf, count, datatype, rank, tag, comm, context_offset, av, status,
                          request);
#else
    if (unlikely(rank == MPI_ANY_SOURCE)) {
        mpi_errno =
            MPIDI_SHM_mpi_irecv(buf, count, datatype, rank, tag, comm, context_offset, request);

        MPIR_ERR_CHECK(mpi_errno);

        MPIDI_REQUEST(*request, is_local) = 1;

        if (!MPIR_Request_is_complete(*request) && !MPIDIG_REQUEST_IN_PROGRESS(*request)) {
            mpi_errno = MPIDI_NM_mpi_irecv(buf, count, datatype, rank, tag, comm, context_offset,
                                           av, &(MPIDI_REQUEST_ANYSOURCE_PARTNER(*request)));

            MPIR_ERR_CHECK(mpi_errno);

            MPIDI_REQUEST(MPIDI_REQUEST_ANYSOURCE_PARTNER(*request), is_local) = 0;

            /* cancel the shm request if netmod/am handles the request from unexpected queue. */
            if (MPIR_Request_is_complete(MPIDI_REQUEST_ANYSOURCE_PARTNER(*request))) {
                mpi_errno = MPIDI_SHM_mpi_cancel_recv(*request);
                if (MPIR_STATUS_GET_CANCEL_BIT((*request)->status)) {
                    (*request)->status = MPIDI_REQUEST_ANYSOURCE_PARTNER(*request)->status;
                }
                MPIR_Request_free(MPIDI_REQUEST_ANYSOURCE_PARTNER(*request));
                goto fn_exit;
            }

            MPIDI_REQUEST_ANYSOURCE_PARTNER(MPIDI_REQUEST_ANYSOURCE_PARTNER(*request)) = *request;
        }
    } else {
        int r;
        if ((r = MPIDI_av_is_local(av)))
            mpi_errno =
                MPIDI_SHM_mpi_recv(buf, count, datatype, rank, tag, comm, context_offset, status,
                                   request);
        else
            mpi_errno =
                MPIDI_NM_mpi_recv(buf, count, datatype, rank, tag, comm, context_offset, av, status,
                                  request);
        if (mpi_errno == MPI_SUCCESS && *request) {
            MPIDI_REQUEST(*request, is_local) = r;
            MPIDI_REQUEST_ANYSOURCE_PARTNER(*request) = NULL;
        }
    }
#endif

    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_RECV_UNSAFE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_irecv_unsafe(void *buf,
                                                MPI_Aint count,
                                                MPI_Datatype datatype,
                                                int rank,
                                                int tag,
                                                MPIR_Comm * comm, int context_offset,
                                                MPIDI_av_entry_t * av, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IRECV_UNSAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IRECV_UNSAFE);


#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno =
        MPIDI_NM_mpi_irecv(buf, count, datatype, rank, tag, comm, context_offset, av, request);
#else
    if (unlikely(rank == MPI_ANY_SOURCE)) {
        mpi_errno =
            MPIDI_SHM_mpi_irecv(buf, count, datatype, rank, tag, comm, context_offset, request);

        MPIR_ERR_CHECK(mpi_errno);

        MPIR_Assert(*request);
        if (!MPIR_Request_is_complete(*request) && !MPIDIG_REQUEST_IN_PROGRESS(*request)) {
            mpi_errno = MPIDI_NM_mpi_irecv(buf, count, datatype, rank, tag, comm, context_offset,
                                           av, &(MPIDI_REQUEST_ANYSOURCE_PARTNER(*request)));

            MPIR_ERR_CHECK(mpi_errno);

            MPIDI_REQUEST(*request, is_local) = 1;
            MPIDI_REQUEST(MPIDI_REQUEST_ANYSOURCE_PARTNER(*request), is_local) = 0;

            /* cancel the shm request if netmod/am handles the request from unexpected queue. */
            if (MPIR_Request_is_complete(MPIDI_REQUEST_ANYSOURCE_PARTNER(*request))) {
                mpi_errno = MPIDI_SHM_mpi_cancel_recv(*request);
                if (MPIR_STATUS_GET_CANCEL_BIT((*request)->status)) {
                    (*request)->status = MPIDI_REQUEST_ANYSOURCE_PARTNER(*request)->status;
                }
                MPIR_Request_free(MPIDI_REQUEST_ANYSOURCE_PARTNER(*request));
                goto fn_exit;
            }

            MPIDI_REQUEST_ANYSOURCE_PARTNER(MPIDI_REQUEST_ANYSOURCE_PARTNER(*request)) = *request;
        }
    } else {
        int r;
        if ((r = MPIDI_av_is_local(av)))
            mpi_errno =
                MPIDI_SHM_mpi_irecv(buf, count, datatype, rank, tag, comm, context_offset, request);
        else
            mpi_errno =
                MPIDI_NM_mpi_irecv(buf, count, datatype, rank, tag, comm, context_offset, av,
                                   request);
        if (mpi_errno == MPI_SUCCESS) {
            MPIR_Assert(*request);
            MPIDI_REQUEST(*request, is_local) = r;
            MPIDI_REQUEST_ANYSOURCE_PARTNER(*request) = NULL;
        }
    }
#endif

    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IRECV_UNSAFE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_imrecv_unsafe(void *buf,
                                                 MPI_Aint count, MPI_Datatype datatype,
                                                 MPIR_Request * message)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IMRECV_UNSAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IMRECV_UNSAFE);

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_imrecv(buf, count, datatype, message);
#else
    if (MPIDI_REQUEST(message, is_local))
        mpi_errno = MPIDI_SHM_mpi_imrecv(buf, count, datatype, message);
    else
        mpi_errno = MPIDI_NM_mpi_imrecv(buf, count, datatype, message);
#endif

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IMRECV_UNSAFE);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_cancel_recv_unsafe(MPIR_Request * rreq)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CANCEL_RECV_UNSAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CANCEL_RECV_UNSAFE);

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_cancel_recv(rreq);
#else
    if (MPIDI_REQUEST(rreq, is_local)) {
        MPIR_Request *partner_rreq = MPIDI_REQUEST_ANYSOURCE_PARTNER(rreq);
        if (unlikely(partner_rreq)) {
            /* Canceling MPI_ANY_SOURCE receive -- first cancel NM recv, then SHM */
            mpi_errno = MPIDI_NM_mpi_cancel_recv(partner_rreq);
            MPIR_ERR_CHECK(mpi_errno);
            MPIR_Request_free(partner_rreq);
        }
        mpi_errno = MPIDI_SHM_mpi_cancel_recv(rreq);
    } else {
        mpi_errno = MPIDI_NM_mpi_cancel_recv(rreq);
    }
#endif
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CANCEL_RECV_UNSAFE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_recv_safe(void *buf,
                                             MPI_Aint count,
                                             MPI_Datatype datatype,
                                             int rank,
                                             int tag,
                                             MPIR_Comm * comm,
                                             int context_offset, MPIDI_av_entry_t * av,
                                             MPI_Status * status, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_RECV_SAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_RECV_SAFE);

#ifdef MPIDI_CH4_USE_WORK_QUEUES
    *(req) = MPIR_Request_create(MPIR_REQUEST_KIND__RECV, 0);
    MPIR_ERR_CHKANDSTMT((*req) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    MPIR_Datatype_add_ref_if_not_builtin(datatype);
    MPIDI_workq_pt2pt_enqueue(RECV, NULL /*send_buf */ , buf, count, datatype,
                              rank, tag, comm, context_offset, av,
                              status, *req, NULL /*flag */ , NULL /*message */ ,
                              NULL /*processed */);
#else
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);
    *(req) = NULL;
    mpi_errno = MPIDI_recv_unsafe(buf, count, datatype, rank, tag, comm,
                                  context_offset, av, status, req);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_RECV_SAFE);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_irecv_safe(void *buf,
                                              MPI_Aint count,
                                              MPI_Datatype datatype,
                                              int rank,
                                              int tag,
                                              MPIR_Comm * comm,
                                              int context_offset, MPIDI_av_entry_t * av,
                                              MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IRECV_SAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IRECV_SAFE);

#ifdef MPIDI_CH4_USE_WORK_QUEUES
    *(req) = MPIR_Request_create(MPIR_REQUEST_KIND__RECV, 0);
    MPIR_ERR_CHKANDSTMT((*req) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    MPIR_Datatype_add_ref_if_not_builtin(datatype);
    MPIDI_workq_pt2pt_enqueue(IRECV, NULL /*send_buf */ , buf, count, datatype,
                              rank, tag, comm, context_offset, av,
                              NULL /*status */ , *req, NULL /*flag */ , NULL /*message */ ,
                              NULL /*processed */);
#else
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);
    *(req) = NULL;
    mpi_errno = MPIDI_irecv_unsafe(buf, count, datatype, rank, tag, comm, context_offset, av, req);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IRECV_SAFE);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_imrecv_safe(void *buf,
                                               MPI_Aint count, MPI_Datatype datatype,
                                               MPIR_Request * message)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IMRECV_SAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IMRECV_SAFE);

#ifdef MPIDI_CH4_USE_WORK_QUEUES
    MPIR_Request *request = MPIR_Request_create(MPIR_REQUEST_KIND__RECV, 0);
    MPIR_ERR_CHKANDSTMT(request == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    MPIR_Datatype_add_ref_if_not_builtin(datatype);
    MPIDI_workq_pt2pt_enqueue(IMRECV, NULL /*send_buf */ , buf, count, datatype,
                              0 /*rank */ , 0 /*tag */ , NULL /*comm */ ,
                              0 /*context_offset */ , NULL /*av */ ,
                              NULL /*status */ , request, NULL /*flag */ ,
                              &message, NULL /*processed */);
#else
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);
    mpi_errno = MPIDI_imrecv_unsafe(buf, count, datatype, message);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IMRECV_SAFE);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_cancel_recv_safe(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CANCEL_RECV_SAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CANCEL_RECV_SAFE);

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);

    mpi_errno = MPIDI_cancel_recv_unsafe(rreq);

    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CANCEL_RECV_SAFE);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}


MPL_STATIC_INLINE_PREFIX int MPID_Recv(void *buf,
                                       MPI_Aint count,
                                       MPI_Datatype datatype,
                                       int rank,
                                       int tag,
                                       MPIR_Comm * comm,
                                       int context_offset, MPI_Status * status,
                                       MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_av_entry_t *av = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_RECV);

    av = MPIDIU_comm_rank_to_av(comm, rank);
    mpi_errno =
        MPIDI_recv_safe(buf, count, datatype, rank, tag, comm, context_offset, av, status, request);

    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_RECV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Recv_init(void *buf,
                                            MPI_Aint count,
                                            MPI_Datatype datatype,
                                            int rank,
                                            int tag,
                                            MPIR_Comm * comm, int context_offset,
                                            MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_RECV_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_RECV_INIT);

    rreq = MPIR_Request_create(MPIR_REQUEST_KIND__PREQUEST_RECV, 0);
    MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");

    *request = rreq;
    rreq->comm = comm;
    MPIR_Comm_add_ref(comm);

    MPIDI_PREQUEST(rreq, buffer) = (void *) buf;
    MPIDI_PREQUEST(rreq, count) = count;
    MPIDI_PREQUEST(rreq, datatype) = datatype;
    MPIDI_PREQUEST(rreq, rank) = rank;
    MPIDI_PREQUEST(rreq, tag) = tag;
    MPIDI_PREQUEST(rreq, context_id) = comm->context_id + context_offset;
    rreq->u.persist.real_request = NULL;
    MPIR_cc_set(rreq->cc_ptr, 0);

    MPIDI_PREQUEST(rreq, p_type) = MPIDI_PTYPE_RECV;
    MPIR_Datatype_add_ref_if_not_builtin(datatype);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_RECV_INIT);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}


MPL_STATIC_INLINE_PREFIX int MPID_Mrecv(void *buf,
                                        MPI_Aint count,
                                        MPI_Datatype datatype, MPIR_Request * message,
                                        MPI_Status * status, MPIR_Request ** rreq)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_MRECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_MRECV);

    *rreq = NULL;

    if (message == NULL) {
        /* treat as though MPI_MESSAGE_NO_PROC was passed */
        MPIR_Status_set_procnull(status);
        mpi_errno = MPI_SUCCESS;
        goto fn_exit;
    }

    MPIR_Assert(message->kind == MPIR_REQUEST_KIND__MPROBE);
    message->kind = MPIR_REQUEST_KIND__RECV;
    *rreq = message;

    mpi_errno = MPIDI_imrecv_safe(buf, count, datatype, message);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_MRECV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Imrecv(void *buf, MPI_Aint count, MPI_Datatype datatype,
                                         MPIR_Request * message, MPIR_Request ** rreqp)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IMRECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IMRECV);

    if (message == NULL) {
        MPIDI_Request_create_null_rreq(*rreqp, mpi_errno, goto fn_fail);
        goto fn_exit;
    }

    MPIR_Assert(message->kind == MPIR_REQUEST_KIND__MPROBE);
    message->kind = MPIR_REQUEST_KIND__RECV;
    *rreqp = message;

    mpi_errno = MPIDI_imrecv_safe(buf, count, datatype, message);
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IMRECV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Irecv(void *buf,
                                        MPI_Aint count,
                                        MPI_Datatype datatype,
                                        int rank,
                                        int tag,
                                        MPIR_Comm * comm, int context_offset,
                                        MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_av_entry_t *av = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IRECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IRECV);

    av = MPIDIU_comm_rank_to_av(comm, rank);
    mpi_errno =
        MPIDI_irecv_safe(buf, count, datatype, rank, tag, comm, context_offset, av, request);

    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IRECV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Cancel_recv(MPIR_Request * rreq)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_CANCEL_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_CANCEL_RECV);

    mpi_errno = MPIDI_cancel_recv_safe(rreq);

    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_CANCEL_RECV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* CH4_RECV_H_INCLUDED */
