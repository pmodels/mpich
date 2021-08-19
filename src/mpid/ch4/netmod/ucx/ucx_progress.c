/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ucx_impl.h"

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
