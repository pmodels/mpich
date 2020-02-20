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

#define MPIDI_UCX_SET_STATUS(status, info) \
    do { \
        MPI_Aint count = info->length; \
        status.MPI_ERROR = MPI_SUCCESS; \
        status.MPI_SOURCE = MPIDI_UCX_get_source(info->sender_tag); \
        status.MPI_TAG = MPIDI_UCX_get_tag(info->sender_tag); \
        MPIR_STATUS_SET_COUNT(status, count); \
    } while (0)

MPL_STATIC_INLINE_PREFIX void MPIDI_UCX_recv_cmpl_cb(void *request, ucs_status_t status,
                                                     ucp_tag_recv_info_t * info)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_UCX_RECV_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_UCX_RECV_CMPL_CB);

    MPIDI_UCX_ucp_request_t *ucp_request = request;
    if (ucp_request->is_set) {
        /* we are completing it asyncronously */
        MPIR_Request *rreq = ucp_request->u.recv.rreq;
        if (unlikely(status == UCS_ERR_CANCELED)) {
            MPIR_STATUS_SET_CANCEL_BIT(rreq->status, TRUE);
        } else {
            MPIDI_UCX_SET_STATUS(rreq->status, info);
            if (unlikely(status == UCS_ERR_MESSAGE_TRUNCATED)) {
                rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;
            }
        }
        MPIDIU_request_complete(rreq);
        MPIDI_UCX_UCP_REQUEST_RELEASE(request);
    } else {
        /* we are completing it unexpectedly */
        ucp_request->is_set = 1;
        MPIDI_UCX_SET_STATUS(ucp_request->u.recv_cb.status, info);
        if (unlikely(status == UCS_ERR_MESSAGE_TRUNCATED)) {
            ucp_request->u.recv_cb.status.MPI_ERROR = MPI_ERR_TRUNCATE;
        }
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_UCX_RECV_CMPL_CB);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_UCX_recv(void *buf,
                                            MPI_Aint count,
                                            MPI_Datatype datatype,
                                            int rank,
                                            int tag, MPIR_Comm * comm,
                                            int context_offset,
                                            MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    size_t data_sz;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPIR_Datatype *dt_ptr;
    uint64_t ucp_tag, tag_mask;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_UCX_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_UCX_RECV);

    tag_mask = MPIDI_UCX_tag_mask(tag, rank);
    ucp_tag = MPIDI_UCX_recv_tag(tag, rank, comm->recvcontext_id + context_offset);
    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    MPIDI_UCX_ucp_request_t *ucp_request;
    if (dt_contig) {
        ucp_request = (void *) ucp_tag_recv_nb(MPIDI_UCX_global.worker,
                                               (char *) buf + dt_true_lb, data_sz,
                                               ucp_dt_make_contig(1),
                                               ucp_tag, tag_mask, &MPIDI_UCX_recv_cmpl_cb);
    } else {
        MPIR_Datatype_ptr_add_ref(dt_ptr);
        ucp_request = (void *) ucp_tag_recv_nb(MPIDI_UCX_global.worker,
                                               buf, count,
                                               dt_ptr->dev.netmod.ucx.ucp_datatype,
                                               ucp_tag, tag_mask, &MPIDI_UCX_recv_cmpl_cb);
    }
    MPIDI_UCX_CHK_REQUEST(ucp_request);

    MPIR_Request *rreq = MPIR_Request_create(MPIR_REQUEST_KIND__RECV, 0);
    MPIR_ERR_CHKANDSTMT((rreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    MPIR_Request_add_ref(rreq);

    if (ucp_request->is_set) {
        /* completed immediately */
        rreq->status = ucp_request->u.recv_cb.status;
        MPIDIU_request_complete(rreq);
        MPIDI_UCX_UCP_REQUEST_RELEASE(ucp_request);
    } else {
        /* will complete by callback */
        ucp_request->is_set = 1;
        ucp_request->u.recv.rreq = rreq;

        /* in case we need call ucp_request_cancel */
        MPIDI_UCX_req_recv_t *p_recv = &MPIDI_UCX_REQ(rreq).recv;
        p_recv->ucp_request = ucp_request;
    }

    *request = rreq;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_UCX_RECV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_imrecv(void *buf,
                                                 MPI_Aint count,
                                                 MPI_Datatype datatype, MPIR_Request * message)
{
    int mpi_errno = MPI_SUCCESS;
    size_t data_sz;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPIDI_UCX_ucp_request_t *ucp_request;
    MPIR_Datatype *dt_ptr;

    MPIDI_UCX_req_mprobe_t *mprobe = &MPIDI_UCX_REQ(message).mprobe;

    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
    if (dt_contig) {
        ucp_request =
            (MPIDI_UCX_ucp_request_t *) ucp_tag_msg_recv_nb(MPIDI_UCX_global.worker,
                                                            (char *) buf + dt_true_lb,
                                                            data_sz,
                                                            ucp_dt_make_contig(1),
                                                            mprobe->message_h,
                                                            &MPIDI_UCX_recv_cmpl_cb);
    } else {
        MPIR_Datatype_ptr_add_ref(dt_ptr);
        ucp_request =
            (MPIDI_UCX_ucp_request_t *) ucp_tag_msg_recv_nb(MPIDI_UCX_global.worker,
                                                            buf, count,
                                                            dt_ptr->dev.netmod.ucx.ucp_datatype,
                                                            mprobe->message_h,
                                                            &MPIDI_UCX_recv_cmpl_cb);
    }
    MPIDI_UCX_CHK_REQUEST(ucp_request);

    MPIR_Request *rreq = message;       /* rename for readability */

    /* Following block is identical as MPIDI_NM_mpi_irecv */

    if (ucp_request->is_set) {
        /* completed immediately */
        rreq->status = ucp_request->u.recv_cb.status;
        MPIDIU_request_complete(rreq);
        MPIDI_UCX_UCP_REQUEST_RELEASE(ucp_request);
    } else {
        /* will complete asynchronously */
        ucp_request->is_set = 1;
        ucp_request->u.recv.rreq = rreq;

        /* in case we need call ucp_request_cancel */
        MPIDI_UCX_req_recv_t *p_recv = &MPIDI_UCX_REQ(rreq).recv;
        p_recv->ucp_request = ucp_request;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_recv(void *buf,
                                               MPI_Aint count,
                                               MPI_Datatype datatype,
                                               int rank,
                                               int tag,
                                               MPIR_Comm * comm,
                                               int context_offset,
                                               MPIDI_av_entry_t * addr,
                                               MPI_Status * status, MPIR_Request ** request)
{
    return MPIDI_UCX_recv(buf, count, datatype, rank, tag, comm, context_offset, addr, request);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_irecv(void *buf,
                                                MPI_Aint count,
                                                MPI_Datatype datatype,
                                                int rank,
                                                int tag,
                                                MPIR_Comm * comm, int context_offset,
                                                MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    return MPIDI_UCX_recv(buf, count, datatype, rank, tag, comm, context_offset, addr, request);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_cancel_recv(MPIR_Request * rreq)
{
    if (!MPIR_Request_is_complete(rreq)) {
        MPIDI_UCX_req_recv_t *p_recv = &MPIDI_UCX_REQ(rreq).recv;
        ucp_request_cancel(MPIDI_UCX_global.worker, p_recv->ucp_request);
    }

    return MPI_SUCCESS;
}

#endif /* UCX_RECV_H_INCLUDED */
