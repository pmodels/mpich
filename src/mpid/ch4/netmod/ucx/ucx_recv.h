/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef UCX_RECV_H_INCLUDED
#define UCX_RECV_H_INCLUDED

#include "ucx_impl.h"

/* when the recv is immediately completed, we need get a request to fill the status,
 * but we need to get request from the correct pool or race condition arises. And we
 * don't know the correct vni!
 * The new ucx API should help: https://github.com/openucx/ucx/pull/5060
 * For now, we malloc a temporary request.
 * FIXME: update with new ucx api
 */
MPL_STATIC_INLINE_PREFIX void MPIDI_UCX_recv_cmpl_cb(void *request, ucs_status_t status,
                                                     ucp_tag_recv_info_t * info)
{
    MPIDI_UCX_ucp_request_t *ucp_request = (MPIDI_UCX_ucp_request_t *) request;
    MPIR_Request *rreq = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_UCX_RECV_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_UCX_RECV_CMPL_CB);

    if (ucp_request->req) {
        rreq = ucp_request->req;
    } else {
        rreq = MPL_malloc(sizeof(MPIR_Request), MPL_MEM_OTHER);
        rreq->status.MPI_ERROR = MPI_SUCCESS;
        MPIR_STATUS_SET_CANCEL_BIT(rreq->status, FALSE);
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

    if (ucp_request->req) {
        MPIDIU_request_complete(rreq);
        ucp_request->req = NULL;
        ucp_request_release(ucp_request);
    } else {
        MPIR_cc_set(&rreq->cc, 0);
        ucp_request->req = rreq;
    }


    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_UCX_RECV_CMPL_CB);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_UCX_mrecv_cmpl_cb(void *request, ucs_status_t status,
                                                      ucp_tag_recv_info_t * info)
{
    MPIDI_UCX_ucp_request_t *ucp_request = (MPIDI_UCX_ucp_request_t *) request;
    MPI_Status *mrecv_status;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_UCX_MRECV_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_UCX_MRECV_CMPL_CB);

    /* complete the request if we have it, or allocate a status object */
    int has_request = (ucp_request->req != NULL);
    if (has_request) {
        MPIR_Request *rreq = ucp_request->req;
        mrecv_status = &rreq->status;
    } else {
        mrecv_status = MPL_malloc(sizeof(MPI_Status), MPL_MEM_BUFFER);
        ucp_request->status = mrecv_status;
    }

    /* populate status fields */
    if (unlikely(status == UCS_ERR_MESSAGE_TRUNCATED)) {
        mrecv_status->MPI_ERROR = MPI_ERR_TRUNCATE;
    } else {
        mrecv_status->MPI_ERROR = MPI_SUCCESS;
        MPIR_STATUS_SET_COUNT(*mrecv_status, info->length);
    }
    mrecv_status->MPI_SOURCE = MPIDI_UCX_get_source(info->sender_tag);
    mrecv_status->MPI_TAG = MPIDI_UCX_get_tag(info->sender_tag);

    /* complete the request */
    if (has_request) {
        MPIR_Request *rreq = ucp_request->req;
        MPIDIU_request_complete(rreq);
        ucp_request->req = NULL;
        ucp_request_release(ucp_request);
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_UCX_MRECV_CMPL_CB);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_UCX_recv(void *buf,
                                            MPI_Aint count,
                                            MPI_Datatype datatype,
                                            int rank,
                                            int tag, MPIR_Comm * comm,
                                            int context_offset,
                                            MPIDI_av_entry_t * addr,
                                            int vni_dst, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    size_t data_sz;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPIR_Datatype *dt_ptr;
    uint64_t ucp_tag, tag_mask;
    MPIR_Request *req = *request;
    MPIDI_UCX_ucp_request_t *ucp_request;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_UCX_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_UCX_RECV);

    tag_mask = MPIDI_UCX_tag_mask(tag, rank);
    ucp_tag = MPIDI_UCX_recv_tag(tag, rank, comm->recvcontext_id + context_offset);
    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    if (dt_contig) {
        ucp_request =
            (MPIDI_UCX_ucp_request_t *) ucp_tag_recv_nb(MPIDI_UCX_global.ctx[vni_dst].worker,
                                                        (char *) buf + dt_true_lb, data_sz,
                                                        ucp_dt_make_contig(1),
                                                        ucp_tag, tag_mask, &MPIDI_UCX_recv_cmpl_cb);
    } else {
        MPIR_Datatype_ptr_add_ref(dt_ptr);
        ucp_request =
            (MPIDI_UCX_ucp_request_t *) ucp_tag_recv_nb(MPIDI_UCX_global.ctx[vni_dst].worker,
                                                        buf, count,
                                                        dt_ptr->dev.netmod.ucx.ucp_datatype,
                                                        ucp_tag, tag_mask, &MPIDI_UCX_recv_cmpl_cb);
    }
    MPIDI_UCX_CHK_REQUEST(ucp_request);

    if (ucp_request->req) {
        if (req == NULL) {
            req = MPIR_Request_create_from_pool(MPIR_REQUEST_KIND__RECV, vni_dst);
            memcpy(&req->status, &((MPIR_Request *) ucp_request->req)->status, sizeof(MPI_Status));
            MPIR_cc_set(&req->cc, 0);
            MPL_free(ucp_request->req);
        } else {
            memcpy(&req->status, &((MPIR_Request *) ucp_request->req)->status, sizeof(MPI_Status));
            MPIR_cc_set(&req->cc, 0);
            MPIR_Request_free_unsafe((MPIR_Request *) ucp_request->req);
        }
        ucp_request->req = NULL;
        ucp_request_release(ucp_request);
    } else {
        if (req == NULL)
            req = MPIR_Request_create_from_pool(MPIR_REQUEST_KIND__RECV, vni_dst);
        MPIR_ERR_CHKANDSTMT((req) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
        MPIR_Request_add_ref(req);
        MPIDI_UCX_REQ(req).ucp_request = ucp_request;
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
                                                 MPI_Aint count,
                                                 MPI_Datatype datatype, MPIR_Request * message)
{
    int mpi_errno = MPI_SUCCESS;
    size_t data_sz;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPIDI_UCX_ucp_request_t *ucp_request;
    MPIR_Datatype *dt_ptr;

    int vci = MPIDI_Request_get_vci(message);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
    if (dt_contig) {
        ucp_request =
            (MPIDI_UCX_ucp_request_t *) ucp_tag_msg_recv_nb(MPIDI_UCX_global.ctx[vci].worker,
                                                            (char *) buf + dt_true_lb,
                                                            data_sz,
                                                            ucp_dt_make_contig(1),
                                                            MPIDI_UCX_REQ(message).message_handler,
                                                            &MPIDI_UCX_mrecv_cmpl_cb);
    } else {
        MPIR_Datatype_ptr_add_ref(dt_ptr);
        ucp_request =
            (MPIDI_UCX_ucp_request_t *) ucp_tag_msg_recv_nb(MPIDI_UCX_global.ctx[vci].worker,
                                                            buf, count,
                                                            dt_ptr->dev.netmod.ucx.ucp_datatype,
                                                            MPIDI_UCX_REQ(message).message_handler,
                                                            &MPIDI_UCX_mrecv_cmpl_cb);
    }
    MPIDI_UCX_CHK_REQUEST(ucp_request);


    if (ucp_request->status) {
        message->status = *(ucp_request->status);
        MPL_free(ucp_request->status);
        MPIDIU_request_complete(message);
        ucp_request->status = NULL;
        ucp_request_release(ucp_request);
    } else {
        MPIDI_UCX_REQ(message).ucp_request = ucp_request;
        ucp_request->req = message;
    }

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
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
    int mpi_errno;

    int vni_dst = MPIDI_UCX_get_vni(DST_VCI_FROM_RECVER, comm, rank, comm->rank, tag);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vni_dst).lock);
    mpi_errno =
        MPIDI_UCX_recv(buf, count, datatype, rank, tag, comm, context_offset, addr, vni_dst,
                       request);
    MPIDI_REQUEST_SET_LOCAL(*request, 0, NULL);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vni_dst).lock);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_irecv(void *buf,
                                                MPI_Aint count,
                                                MPI_Datatype datatype,
                                                int rank,
                                                int tag,
                                                MPIR_Comm * comm, int context_offset,
                                                MPIDI_av_entry_t * addr, MPIR_Request ** request,
                                                MPIR_Request * partner)
{
    int mpi_errno;

    int vni_dst = MPIDI_UCX_get_vni(DST_VCI_FROM_RECVER, comm, rank, comm->rank, tag);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vni_dst).lock);
    mpi_errno =
        MPIDI_UCX_recv(buf, count, datatype, rank, tag, comm, context_offset, addr, vni_dst,
                       request);
    MPIDI_REQUEST_SET_LOCAL(*request, 0, partner);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vni_dst).lock);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_cancel_recv(MPIR_Request * rreq)
{
    if (!MPIR_Request_is_complete(rreq)) {
        int vci = MPIDI_Request_get_vci(rreq);
        ucp_request_cancel(MPIDI_UCX_global.ctx[vci].worker, MPIDI_UCX_REQ(rreq).ucp_request);
    }

    return MPI_SUCCESS;
}

#endif /* UCX_RECV_H_INCLUDED */
