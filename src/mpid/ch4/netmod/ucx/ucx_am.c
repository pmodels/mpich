/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ucx_impl.h"

/* Am handler for messages sent from ucp_am_send_nb. Registered with
 * ucp_worker_set_am_handler.
 */
ucs_status_t MPIDI_UCX_am_handler(void *arg, void *data, size_t length, ucp_ep_h reply_ep,
                                  unsigned flags)
{
    void *tmp, *p_data;
    size_t data_sz;

    /* need to copy the message data for alignment purposes */
    tmp = MPL_malloc(length, MPL_MEM_BUFFER);
    MPIR_Memcpy(tmp, data, length);
    MPIDI_UCX_am_header_t *msg_hdr = tmp;
    p_data = (char *) msg_hdr->payload + (length - msg_hdr->data_sz - sizeof(*msg_hdr));
    data_sz = msg_hdr->data_sz;

    int attr = 0;               /* is_local = 0, is_async = 0 */
    MPIDIG_AM_ATTR_SET_VCIS(attr, msg_hdr->src_vci, msg_hdr->dst_vci);
    MPIDIG_global.target_msg_cbs[msg_hdr->handler_id] (msg_hdr->payload,
                                                       p_data, data_sz, attr, NULL);

    MPL_free(tmp);
    return UCS_OK;
}

/* callback for ucp_am_send_nb, used in MPIDI_NM_am_isend.
 * The union request was set to sreq.
 */
void MPIDI_UCX_am_isend_callback(void *request, ucs_status_t status)
{
    MPIDI_UCX_ucp_request_t *ucp_request = (MPIDI_UCX_ucp_request_t *) request;
    MPIR_Request *req = ucp_request->req;
    int handler_id = MPIDI_UCX_AM_SEND_REQUEST(req, handler_id);

    MPIR_FUNC_ENTER;

    MPIR_gpu_free_host(MPIDI_UCX_AM_SEND_REQUEST(req, pack_buffer));
    MPIDI_UCX_AM_SEND_REQUEST(req, pack_buffer) = NULL;
    MPIDIG_global.origin_cbs[handler_id] (req);
    ucp_request->req = NULL;

    MPIR_FUNC_EXIT;
}

/* callback for ucp_am_send_nb, used in MPIDI_NM_am_send_hdr.
 * The union request was set to the send buffer.
 */
void MPIDI_UCX_am_send_callback(void *request, ucs_status_t status)
{
    MPIDI_UCX_ucp_request_t *ucp_request = (MPIDI_UCX_ucp_request_t *) request;

    MPIR_FUNC_ENTER;

    MPL_free(ucp_request->buf);
    ucp_request->buf = NULL;
    ucp_request_release(ucp_request);

    MPIR_FUNC_EXIT;
}

#ifdef HAVE_UCP_AM_NBX
/* Am handler for messages sent from ucp_am_send_nbx. Registered with
 * ucp_worker_set_am_recv_handler.
 */
ucs_status_t MPIDI_UCX_am_nbx_handler(void *arg, const void *header, size_t header_length,
                                      void *data, size_t length, const ucp_am_recv_param_t * param)
{
    /* need to copy the message data for alignment purposes */
    void *tmp = MPL_malloc(header_length, MPL_MEM_BUFFER);
    MPIR_Memcpy(tmp, header, header_length);
    MPIDI_UCX_am_header_t *msg_hdr = tmp;

    int attr = 0;
    MPIDIG_AM_ATTR_SET_VCIS(attr, msg_hdr->src_vci, msg_hdr->dst_vci);

    bool got_data = !(param->recv_attr & UCP_AM_RECV_ATTR_FLAG_RNDV);
    if (got_data) {
        MPIDIG_global.target_msg_cbs[msg_hdr->handler_id] (msg_hdr->payload,
                                                           data, length, attr, NULL);
        MPL_free(tmp);
        return UCS_OK;
    } else {
        MPIR_Request *rreq;
        attr |= MPIDIG_AM_ATTR__IS_ASYNC | MPIDIG_AM_ATTR__IS_RNDV;
        MPIDIG_global.target_msg_cbs[msg_hdr->handler_id] (msg_hdr->payload, NULL, 0, attr, &rreq);
        MPL_free(tmp);
        if (!rreq) {
            /* ignoring data */
            return UCS_OK;
        } else {
            MPIDI_UCX_AM_RECV_REQUEST(rreq, data_desc) = data;
            MPIDI_UCX_AM_RECV_REQUEST(rreq, data_sz) = length;
            if (MPIDIG_recv_initialized(rreq)) {
                /* recv buffer ready, proceed */
                MPIDI_UCX_do_am_recv(rreq);
                return UCS_OK;
            } else {
                /* will wait for ch4 layer call data_copy_cb */
                return UCS_INPROGRESS;
            }
        }
    }
}

/* Called when recv buffer is posted */
int MPIDI_UCX_do_am_recv(MPIR_Request * rreq)
{
    void *recv_buf;
    bool is_contig;
    MPI_Aint data_sz, in_data_sz;
    int vci = MPIDI_Request_get_vci(rreq);

    MPIDIG_get_recv_buffer(&recv_buf, &data_sz, &is_contig, &in_data_sz, rreq);
    if (!is_contig || in_data_sz > data_sz) {
        /* non-contig datatype, need receive into pack buffer */
        /* ucx will error out if buffer size is less than the promised data size,
         * also use a pack buffer in this case */
        recv_buf = MPL_malloc(in_data_sz, MPL_MEM_OTHER);
        MPIR_Assert(recv_buf);
        MPIDI_UCX_AM_RECV_REQUEST(rreq, pack_buffer) = recv_buf;
    } else {
        MPIDI_UCX_AM_RECV_REQUEST(rreq, pack_buffer) = NULL;
    }

    MPIDI_UCX_ucp_request_t *ucp_request;
    size_t received_length;
    ucp_request_param_t param = {
        .op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK | UCP_OP_ATTR_FIELD_RECV_INFO,
        .cb.recv_am = &MPIDI_UCX_am_recv_callback_nbx,
        .recv_info.length = &received_length,
    };
    void *data_desc = MPIDI_UCX_AM_RECV_REQUEST(rreq, data_desc);
    /* note: use in_data_sz to match promised data size */
    ucp_request = ucp_am_recv_data_nbx(MPIDI_UCX_global.ctx[vci].worker,
                                       data_desc, recv_buf, in_data_sz, &param);
    if (ucp_request == NULL) {
        /* completed immediately */
        MPIDI_UCX_ucp_request_t tmp_ucp_request;
        tmp_ucp_request.req = rreq;
        MPIDI_UCX_am_recv_callback_nbx(&tmp_ucp_request, UCS_OK, received_length, NULL);
    } else {
        ucp_request->req = rreq;
    }

    return MPI_SUCCESS;
}

/* callback for ucp_am_recv_data_nbx */
void MPIDI_UCX_am_recv_callback_nbx(void *request, ucs_status_t status, size_t length,
                                    void *user_data)
{
    MPIDI_UCX_ucp_request_t *ucp_request = (MPIDI_UCX_ucp_request_t *) request;
    MPIR_Request *rreq = ucp_request->req;

    /* FIXME: proper error handling */
    MPIR_Assert(status == UCS_OK);

    if (MPIDI_UCX_AM_RECV_REQUEST(rreq, pack_buffer)) {
        MPIDIG_recv_copy(MPIDI_UCX_AM_RECV_REQUEST(rreq, pack_buffer), rreq);
        MPL_free(MPIDI_UCX_AM_RECV_REQUEST(rreq, pack_buffer));
        MPIDI_UCX_AM_RECV_REQUEST(rreq, pack_buffer) = NULL;
    } else {
        MPIDIG_recv_done(length, rreq);
    }
    MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);
    ucp_request->req = NULL;
    ucp_request_release(ucp_request);
}

void MPIDI_UCX_am_isend_callback_nbx(void *request, ucs_status_t status, void *user_data)
{
    /* note: only difference from MPIDI_UCX_am_isend_callback is we need
     * MPL_free in stead of MPIR_gpu_free_host
     */
    MPIDI_UCX_ucp_request_t *ucp_request = (MPIDI_UCX_ucp_request_t *) request;
    MPIR_Request *req = ucp_request->req;
    int handler_id = MPIDI_UCX_AM_SEND_REQUEST(req, handler_id);

    MPL_free(MPIDI_UCX_AM_SEND_REQUEST(req, pack_buffer));
    MPIDI_UCX_AM_SEND_REQUEST(req, pack_buffer) = NULL;
    MPIDIG_global.origin_cbs[handler_id] (req);
    ucp_request->req = NULL;
}
#endif
