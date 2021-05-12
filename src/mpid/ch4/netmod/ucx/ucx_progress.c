/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ucx_impl.h"

ucs_status_t MPIDI_UCX_am_handler_short(void *arg, void *data, size_t length, ucp_ep_h reply_ep,
                                        unsigned flags)
{
    void *tmp, *p_data;
    size_t data_sz;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_UCX_AM_HANDLER_SHORT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_UCX_AM_HANDLER_SHORT);

    /* need to copy the message data for alignment purposes */
    tmp = MPL_malloc(length, MPL_MEM_BUFFER);
    MPIR_Memcpy(tmp, data, length);
    MPIDI_UCX_am_header_t *msg_hdr = tmp;
    p_data = (char *) msg_hdr->payload + (length - msg_hdr->data_sz - sizeof(*msg_hdr));
    data_sz = msg_hdr->data_sz;

    /* note: setting is_local, is_async to 0, 0 */
    MPIDIG_global.target_msg_cbs[msg_hdr->handler_id] (msg_hdr->handler_id, msg_hdr->payload,
                                                       p_data, data_sz, 0, 0, NULL);

    MPL_free(tmp);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_UCX_AM_HANDLER_SHORT);
    return UCS_OK;
}

ucs_status_t MPIDI_UCX_am_handler_pipeline(void *arg, void *data, size_t length, ucp_ep_h reply_ep,
                                           unsigned flags)
{
    void *tmp, *p_data;
    int is_done = 0;
    MPIR_Request *rreq = NULL;
    MPIR_Request *cache_rreq = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_UCX_AM_HANDLER_PIPELINE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_UCX_AM_HANDLER_PIPELINE);

    /* need to copy the message data for alignment purposes */
    tmp = MPL_malloc(length, MPL_MEM_BUFFER);
    MPIR_Memcpy(tmp, data, length);
    MPIDI_UCX_am_header_t *msg_hdr = tmp;
    p_data = (char *) msg_hdr->payload + (length - msg_hdr->data_sz - sizeof(*msg_hdr));

    cache_rreq = MPIDIG_req_cache_lookup(MPIDI_UCX_global.req_map, (uint64_t) msg_hdr->src_grank);
    rreq = cache_rreq;

    if (!rreq) {
        /* note: setting is_local, is_async to 0, 0 */
        MPIDIG_global.target_msg_cbs[msg_hdr->handler_id] (msg_hdr->handler_id, msg_hdr->payload,
                                                           p_data, msg_hdr->data_sz, 0, 1, &rreq);
        MPIDIG_recv_setup(rreq);
        MPIDIG_req_cache_add(MPIDI_UCX_global.req_map, (uint64_t) msg_hdr->src_grank, rreq);
    }

    is_done = MPIDIG_recv_copy_seg(p_data, msg_hdr->data_sz, rreq);
    if (is_done) {
        MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);
        MPIDIG_req_cache_remove(MPIDI_UCX_global.req_map, (uint64_t) msg_hdr->src_grank);
    }

    MPL_free(tmp);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_UCX_AM_HANDLER_PIPELINE);
    return UCS_OK;
}
