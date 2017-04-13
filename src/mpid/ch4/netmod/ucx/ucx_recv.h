/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Mellanox Technologies Ltd.
 *  Copyright (C) Mellanox Technologies Ltd. 2016. ALL RIGHTS RESERVED
 */
#ifndef UCX_RECV_H_INCLUDED
#define UCX_RECV_H_INCLUDED

#include "ucx_impl.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_UCX_recv_cmpl_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void MPIDI_UCX_recv_cmpl_cb(void *request, ucs_status_t status,
                                          ucp_tag_recv_info_t * info)
{
    MPIDI_UCX_ucp_request_t *ucp_request = (MPIDI_UCX_ucp_request_t *) request;
    MPIR_Request *rreq = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_UCX_RECV_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_UCX_RECV_CMPL_CB);

    if (ucp_request->req) {
        rreq = ucp_request->req;
        MPIDI_CH4U_request_complete(rreq);
        ucp_request->req = NULL;
        ucp_request_release(ucp_request);
    } else {
        rreq = MPIR_Request_create(MPIR_REQUEST_KIND__RECV);
        MPIR_cc_set(&rreq->cc, 0);
        ucp_request->req = rreq;
    }

    if (unlikely(status == UCS_ERR_CANCELED)) {
        MPIR_STATUS_SET_CANCEL_BIT(rreq->status, TRUE);
    } else if (unlikely(status == UCS_ERR_MESSAGE_TRUNCATED)) {
        rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;
        rreq->status.MPI_SOURCE = MPIDI_UCX_get_source(info->sender_tag);
        rreq->status.MPI_TAG = MPIDI_UCX_get_tag(info->sender_tag);
    } else {
        MPI_Aint count = info->length;
        rreq->status.MPI_ERROR = MPI_SUCCESS;
        rreq->status.MPI_SOURCE = MPIDI_UCX_get_source(info->sender_tag);
        rreq->status.MPI_TAG = MPIDI_UCX_get_tag(info->sender_tag);
        MPIR_STATUS_SET_COUNT(rreq->status, count);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_UCX_RECV_CMPL_CB);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_UCX_mrecv_cmpl_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void MPIDI_UCX_mrecv_cmpl_cb(void *request, ucs_status_t status,
                                           ucp_tag_recv_info_t * info)
{
    MPIDI_UCX_ucp_request_t *ucp_request = (MPIDI_UCX_ucp_request_t *) request;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_UCX_MRECV_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_UCX_MRECV_CMPL_CB);

    if (ucp_request->req) {
        MPIR_Request *rreq = ucp_request->req;
        MPIDI_CH4U_request_complete(rreq);
        ucp_request->req = NULL;
        ucp_request_release(ucp_request);

        if (unlikely(status == UCS_ERR_MESSAGE_TRUNCATED)) {
            rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;
            rreq->status.MPI_SOURCE = MPIDI_UCX_get_source(info->sender_tag);
            rreq->status.MPI_TAG = MPIDI_UCX_get_tag(info->sender_tag);
        } else {
            rreq->status.MPI_ERROR = MPI_SUCCESS;
            rreq->status.MPI_SOURCE = MPIDI_UCX_get_source(info->sender_tag);
            rreq->status.MPI_TAG = MPIDI_UCX_get_tag(info->sender_tag);
            MPIR_STATUS_SET_COUNT(rreq->status, info->length);
        }
    } else {
        if (unlikely(status == UCS_ERR_MESSAGE_TRUNCATED)) {
            /* FIXME: we have no way of passing the tag bits back in this case */
            ucp_request->req = (void *)UCS_ERR_MESSAGE_TRUNCATED;
        } else {
            ucp_request->req = MPL_malloc(sizeof(ucp_tag_recv_info_t));
            memcpy(ucp_request->req, info, sizeof(ucp_tag_recv_info_t));
        }
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_UCX_MRECV_CMPL_CB);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_UCX_recv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_UCX_recv(void *buf,
                                 int count,
                                 MPI_Datatype datatype,
                                 int rank,
                                 int tag, MPIR_Comm * comm,
                                 int context_offset,
                                 MPIDI_av_entry_t *addr,
                                 MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    size_t data_sz;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPIR_Datatype *dt_ptr;
    uint64_t ucp_tag, tag_mask;
    MPIR_Request *req;
    MPIDI_UCX_ucp_request_t *ucp_request;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_UCX_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_UCX_RECV);

    tag_mask = MPIDI_UCX_tag_mask(tag, rank);
    ucp_tag = MPIDI_UCX_recv_tag(tag, rank, comm->recvcontext_id + context_offset);
    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    if (dt_contig) {
        ucp_request =
            (MPIDI_UCX_ucp_request_t *)ucp_tag_recv_nb(MPIDI_UCX_global.worker,
                                                       (char *)buf + dt_true_lb, data_sz,
                                                       ucp_dt_make_contig(1),
                                                       ucp_tag, tag_mask,
                                                       &MPIDI_UCX_recv_cmpl_cb);
    } else {
        MPIDU_Datatype_add_ref(dt_ptr);
        ucp_request =
            (MPIDI_UCX_ucp_request_t *)ucp_tag_recv_nb(MPIDI_UCX_global.worker,
                                                       buf, count,
                                                       dt_ptr->dev.netmod.ucx.ucp_datatype,
                                                       ucp_tag, tag_mask,
                                                       &MPIDI_UCX_recv_cmpl_cb);
    }
    MPIDI_CH4_UCX_REQUEST(ucp_request);

    if (ucp_request->req) {
        req = ucp_request->req;
        ucp_request->req = NULL;
        ucp_request_release(ucp_request);
    } else {
        req = MPIR_Request_create(MPIR_REQUEST_KIND__RECV);
        MPIR_Request_add_ref(req);
        MPIDI_UCX_REQ(req).a.ucp_request = ucp_request;
        ucp_request->req = req;
    }
    *request = req;

 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_UCX_RECV);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_imrecv(void *buf,
                                                 int count,
                                                 MPI_Datatype datatype,
                                                 MPIR_Request * message, MPIR_Request ** rreqp)
{
    int mpi_errno = MPI_SUCCESS;
    size_t data_sz;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPIDI_UCX_ucp_request_t *ucp_request;
    MPIR_Datatype *dt_ptr;

    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
    if (dt_contig) {
        ucp_request =
            (MPIDI_UCX_ucp_request_t *)ucp_tag_msg_recv_nb(MPIDI_UCX_global.worker,
                                                           (char *) buf + dt_true_lb,
                                                           data_sz,
                                                           ucp_dt_make_contig(1),
                                                           MPIDI_UCX_REQ(message).a.message_handler,
                                                           &MPIDI_UCX_mrecv_cmpl_cb);
    } else {
        MPIDU_Datatype_add_ref(dt_ptr);
        ucp_request =
            (MPIDI_UCX_ucp_request_t *)ucp_tag_msg_recv_nb(MPIDI_UCX_global.worker,
                                                           buf, count,
                                                           dt_ptr->dev.netmod.ucx.ucp_datatype,
                                                           MPIDI_UCX_REQ(message).a.message_handler,
                                                           &MPIDI_UCX_mrecv_cmpl_cb);
    }
    MPIDI_CH4_UCX_REQUEST(ucp_request);

    if (ucp_request->req) {
        if (unlikely((ucs_status_t)ucp_request->req == UCS_ERR_MESSAGE_TRUNCATED)) {
            message->status.MPI_ERROR = MPI_ERR_TRUNCATE;
        } else {
            ucp_tag_recv_info_t *info = ucp_request->req;
            message->status.MPI_ERROR = MPI_SUCCESS;
            message->status.MPI_SOURCE = MPIDI_UCX_get_source(info->sender_tag);
            message->status.MPI_TAG = MPIDI_UCX_get_tag(info->sender_tag);
            MPIR_STATUS_SET_COUNT(message->status, info->length);
            MPL_free(ucp_request->req);
        }
        MPIDI_CH4U_request_complete(message);
        ucp_request->req = NULL;
        ucp_request_release(ucp_request);
    } else {
        MPIDI_UCX_REQ(message).a.ucp_request = ucp_request;
        ucp_request->req = message;
    }
    message->kind = MPIR_REQUEST_KIND__RECV;
    *rreqp = message;

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_recv(void *buf,
                                               int count,
                                               MPI_Datatype datatype,
                                               int rank,
                                               int tag,
                                               MPIR_Comm * comm,
                                               int context_offset,
                                               MPIDI_av_entry_t *addr,
                                               MPI_Status * status, MPIR_Request ** request)
{
    return MPIDI_UCX_recv(buf, count, datatype, rank, tag, comm, context_offset,
                          addr, request);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_irecv(void *buf,
                                                int count,
                                                MPI_Datatype datatype,
                                                int rank,
                                                int tag,
                                                MPIR_Comm * comm, int context_offset,
                                                MPIDI_av_entry_t *addr,
                                                MPIR_Request ** request)
{
    return MPIDI_UCX_recv(buf, count, datatype, rank, tag, comm, context_offset,
                          addr, request);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_recv_init(void *buf,
                                                    int count,
                                                    MPI_Datatype datatype,
                                                    int rank,
                                                    int tag,
                                                    MPIR_Comm * comm,
                                                    int context_offset,
                                                    MPIDI_av_entry_t *addr,
                                                    MPIR_Request ** request)
{
    return MPIDIG_mpi_recv_init(buf, count, datatype, rank, tag, comm, context_offset, request);
}

static inline int MPIDI_NM_mpi_cancel_recv(MPIR_Request * rreq)
{
    if (!MPIR_Request_is_complete(rreq)) {
        ucp_request_cancel(MPIDI_UCX_global.worker, MPIDI_UCX_REQ(rreq).a.ucp_request);
    }

    return MPI_SUCCESS;
}

#endif /* UCX_RECV_H_INCLUDED */
