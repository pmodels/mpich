/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ucx_impl.h"

void *MPIDI_UCX_am_buf;         /* message buffer has global scope because ucx executes am_handler */

void MPIDI_UCX_am_handler(void *request, ucs_status_t status, ucp_tag_recv_info_t * info)
{
    void *p_data;
    void *in_data;
    size_t data_sz;
    MPIDI_UCX_am_header_t *msg_hdr = (MPIDI_UCX_am_header_t *) MPIDI_UCX_am_buf;

    p_data = in_data =
        (char *) msg_hdr->payload + (info->length - msg_hdr->data_sz - sizeof(*msg_hdr));
    data_sz = msg_hdr->data_sz;

    /* note: setting is_local, is_async to 0, 0 */
    MPIDIG_global.target_msg_cbs[msg_hdr->handler_id] (msg_hdr->handler_id, msg_hdr->payload,
                                                       p_data, data_sz, 0, 0, NULL);

}
