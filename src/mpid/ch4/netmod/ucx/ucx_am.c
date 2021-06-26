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
    int handler_id = req->dev.ch4.am.netmod_am.ucx.handler_id;

    MPIR_FUNC_ENTER;

    MPIR_gpu_free_host(req->dev.ch4.am.netmod_am.ucx.pack_buffer);
    req->dev.ch4.am.netmod_am.ucx.pack_buffer = NULL;
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
